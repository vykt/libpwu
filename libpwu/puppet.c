#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sched.h>

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/mman.h>

#include <asm/unistd.h>

#include "libpwu.h"
#include "puppet.h"
#include "pattern.h"


//sends SIGSTOP itself, don't call util's sig_stop()
int puppet_attach(puppet_info p_info) {

	long ret;
	pid_t ret_pid;
	time_t timeout;

	int wait_status;

	ret = ptrace(PTRACE_ATTACH, p_info.pid, NULL, NULL);
	if (ret == -1) return -1;

	ret_pid = waitpid(p_info.pid, &wait_status, 0);
	if (ret_pid == -1) return -1;

	//wait for process to stop
	timeout = time(NULL);
	while (1) {
		if (WIFSTOPPED(wait_status)) return 0;
		if (timeout + STOP_TIMEOUT >= timeout) {
			return -1;
		}
	}//end wait for process to stop
}


//detach and send SIGCONT
int puppet_detach(puppet_info p_info) {

	long ret;

	//send detach call
	ret = ptrace(PTRACE_DETACH, p_info.pid, NULL, NULL);
	if (ret == -1) return -1;

	return 0;
}


//find syscall instruction in process
int puppet_find_syscall(puppet_info * p_info, maps_data * m_data, int fd_mem) {

    int ret;
    unsigned int syscall_offset;
    pattern ptn;
    maps_entry * m_entry_ref;

	//create new pattern object to search for a syscall
	ret = new_pattern(&ptn, NULL, (byte *) "\x0f\x05", 2);
	if (ret == -1) return -2;

	//for every region
	for (int i = 0; i < m_data->entry_vector.length; ++i) {

		//get region
		ret = vector_get_ref(&m_data->entry_vector, i, (byte **) &m_entry_ref);
		if (ret == -1) return -2;

		//continue if this region is not executable
		if (m_entry_ref->perms < PROT_EXEC ) continue;

		//search the region
		ptn.search_region = m_entry_ref;
		ret = match_pattern(&ptn, fd_mem);

        //pattern match failed
        if (ret == -1) {
            del_pattern(&ptn);
            return -2;
        //syscall not found
        } else if (ret == 0) {
            continue;
        //syscall found
        } else {
            ret = vector_get(&ptn.offset_vector, 0, (byte *) &syscall_offset);
            if (ret == -1) {
                del_pattern(&ptn);
                return -2;
            }
            p_info->syscall_addr = m_entry_ref->start_addr + syscall_offset;
            return 0;
        }//end search result

    }//end for every region

    return -1;
}


//save registers of puppeted process
int puppet_save_regs(puppet_info * p_info) {

	long ret;

	//get process registers
	ret = ptrace(PTRACE_GETREGS, p_info->pid, NULL, &p_info->saved_state);
	if (ret == -1) return -1;

	ret = ptrace(PTRACE_GETFPREGS, p_info->pid, NULL, &p_info->saved_float_state);
	if (ret == -1) return -1;

	return 0;
}


//write new register values to puppeted process
int puppet_write_regs(puppet_info * p_info) {

	long ret;

	ret = ptrace(PTRACE_SETREGS, p_info->pid, NULL, &p_info->new_state);
	if (ret == -1) return -1;

	ret = ptrace(PTRACE_SETFPREGS, p_info->pid, NULL, &p_info->saved_float_state);
	if (ret == -1) return -1;

	return 0;
}


//copy registers
void puppet_copy_regs(puppet_info * p_info, int mode) {

    struct user_regs_struct * dest_regs;
    struct user_regs_struct * src_regs;
    struct user_fpregs_struct * dest_fp_regs;
    struct user_fpregs_struct * src_fp_regs;

    //if copying new registers to old
    if (mode == COPY_NEW) {
        dest_regs = &p_info->saved_state;
        src_regs = &p_info->new_state;
        dest_fp_regs = &p_info->saved_float_state;
        src_fp_regs = &p_info->new_float_state;
    //else if copying old registers to new
    } else {
        dest_regs = &p_info->new_state;
        src_regs = &p_info->saved_state;
        dest_fp_regs = &p_info->new_float_state;
        src_fp_regs = &p_info->saved_float_state;
    }

    memcpy(dest_regs, src_regs, sizeof(struct user_regs_struct));
    memcpy(dest_fp_regs, src_fp_regs, sizeof(struct user_fpregs_struct));
}


//generic function for calling an arbitrary syscall
//p_info->new_state needs to be set by caller
int arbitrary_syscall(puppet_info * p_info, int fd_mem,
                      unsigned long long * syscall_ret) {

    int ret;
    long ret_long;
    long ptrace_ret;
    struct user_regs_struct temp_state;

    //save registers
    ret = puppet_save_regs(p_info);
    if (ret == -1) return -1;

    //write new registers
    ret = puppet_write_regs(p_info);
    if (ret == -1) return -1;

    //catch entry and exit of syscall
    ptrace_ret = ptrace(PTRACE_SYSCALL, p_info->pid, NULL, NULL);
    if (ptrace_ret == -1) return -1;
    waitpid(p_info->pid, 0, 0);

    ptrace_ret = ptrace(PTRACE_SYSCALL, p_info->pid, NULL, NULL);
    if (ptrace_ret == -1) return -1;
    waitpid(p_info->pid, 0, 0);

    //if caller asked for return value, fetch it
    if (syscall_ret != NULL) {
        ret_long = ptrace(PTRACE_GETREGS, p_info->pid, NULL, &temp_state);
        if (ret_long == -1) return -1;
        *syscall_ret = temp_state.rax;
    }

    //now restore registers
    memcpy(&p_info->new_state, &p_info->saved_state,
           sizeof(struct user_regs_struct));
    memcpy(&p_info->new_float_state, &p_info->saved_float_state,
           sizeof(struct user_fpregs_struct));
    ret = puppet_write_regs(p_info);
    if (ret == -1) return -1;

    return 0;
}


//change permissions of a region
//perms = mprotect permission bits (man 2 mprotect)
//target_region is a REFERENCE to a region inside target_maps
int change_region_perms(puppet_info * p_info, byte perms, int fd_mem,
		                maps_entry * target_region) {

	int ret;

    //get registers & copy them
    ret = puppet_save_regs(p_info);
    if (ret == -1) return -1;
    puppet_copy_regs(p_info, COPY_OLD);

    //setup registers for the call to mprotect
    p_info->new_state.rax = __NR_mprotect; //mprotect syscall number
    p_info->new_state.rdi = (unsigned long long) target_region->start_addr;
    p_info->new_state.rsi = target_region->end_addr - target_region->start_addr;
    p_info->new_state.rdx = perms;
    p_info->new_state.rip = (unsigned long long) p_info->syscall_addr;

    //call arbitrary syscall
    ret = arbitrary_syscall(p_info, fd_mem, NULL);
    if (ret == -1) return -1;

    //update own memory map to new value
    target_region->perms = perms;

    return 0;
}


//create new anonymous map inside target process
int create_thread_stack(puppet_info * p_info, int fd_mem, void ** stack_addr,
                        unsigned int stack_size) {

    int ret;

    //get registers & copy them
    ret = puppet_save_regs(p_info);
    if (ret == -1) return -1;
    puppet_copy_regs(p_info, COPY_OLD);

    //setup registers for te call to mmap
    p_info->new_state.rax = __NR_mmap; //mmap syscall number
    p_info->new_state.rdi = 0;         //any destination
    p_info->new_state.rsi = (unsigned long long) stack_size;
    p_info->new_state.rdx = PROT_READ | PROT_WRITE;
    p_info->new_state.r10 = MAP_PRIVATE | MAP_ANONYMOUS;
    p_info->new_state.r9  = 0;
    p_info->new_state.r8  = -1;
    p_info->new_state.rip = (unsigned long long) p_info->syscall_addr;

    //call arbitrary syscall
    ret = arbitrary_syscall(p_info, fd_mem, (unsigned long long *) stack_addr);
    if (ret == -1) return -1;

    return 0;
}


//setup thread payload
int setup_new_thread_payload(int fd_mem, new_thread_setup n_t_setup) {

	//offsets specific to the new_thread.o auto payload
	int rbp_offset = 0xC;
	int rsp_offset = 0x16;
	int jmp_offset = 0x20;

	int ret;
    void * thread_func_addr;

	mutation m;
	raw_injection r_injection;



	//read payload
	ret = new_raw_injection(&r_injection, n_t_setup.setup_region,
	                        n_t_setup.setup_offset, new_thread_payload);
	if (ret == -1) return -1;

    //initialise mutation structure
    ret = new_mutation(&m, sizeof(void *));
    if (ret == -1) return -1;

	//add first mutation for RBP
	m.offset = rbp_offset;
	memcpy(m.mod, &n_t_setup.stack_addr, sizeof(void *));
	m.mod_len = sizeof(void *);

	//mutate payload with RBP mutation
	ret = apply_mutation(r_injection.payload, &m);
	if (ret == -1) return -1;

	//add second mutation for RSP
	m.offset = rsp_offset;
	memcpy(m.mod, &n_t_setup.stack_addr, sizeof(void *));
	m.mod_len = sizeof(void *);

	//mutate payload with RSP mutation
	ret = apply_mutation(r_injection.payload, &m);
	if (ret == -1) return -1;

	//add third mutation for jump to thread function
    thread_func_addr = n_t_setup.thread_func_region->start_addr 
                       + n_t_setup.thread_func_offset;
	m.offset = jmp_offset;
	memcpy(m.mod, &thread_func_addr, sizeof(void *));
	m.mod_len = sizeof(void *);

	//mutate payload with thread function call mutation
	ret = apply_mutation(r_injection.payload, &m);
	if (ret == -1) return -1;

	//inject payload into process
	ret = raw_inject(r_injection, fd_mem);
	if (ret == -1) return -1;

	//clean up
	del_raw_injection(&r_injection);
    del_mutation(&m);

	return 0;
}


//start a new thread, executing function starting at exec_addr
int start_thread(puppet_info * p_info, int fd_mem, new_thread_setup n_t_setup, 
                 int * tid) {

    int ret;
	void * real_stack_addr;

	//stack grows down, so get the end address
	real_stack_addr = n_t_setup.stack_addr + n_t_setup.stack_size - 0x1000; //-0x8;
    n_t_setup.stack_addr = real_stack_addr;

	//setup the bootstrap payload for a new thread
	ret = setup_new_thread_payload(fd_mem, n_t_setup);
	if (ret == -1) return -1;

    //get registers & copy them
    ret = puppet_save_regs(p_info);
    if (ret == -1) return -1;
    puppet_copy_regs(p_info, COPY_OLD);

    //setup registers for call to clone
    p_info->new_state.rax = __NR_clone; //clone syscall number
    p_info->new_state.rdi = CLONE_VM | CLONE_SIGHAND | CLONE_THREAD
                            | CLONE_FS | CLONE_FILES | CLONE_PARENT | CLONE_IO;
    p_info->new_state.rsi = (unsigned long long)(n_t_setup.stack_addr);
    p_info->new_state.rip = (unsigned long long)(n_t_setup.setup_region->start_addr
	                                             + n_t_setup.setup_offset);

    //call arbitrary syscall
    ret = arbitrary_syscall(p_info, fd_mem, (unsigned long long *) tid);
    if (ret == -1) return -1;

    return 0;
}
