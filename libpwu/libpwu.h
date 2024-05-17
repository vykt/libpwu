#ifndef _LIBPWU_H
#define _LIBPWU_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stdint.h>

#include <sys/user.h>
#include <sys/types.h>

#include <linux/limits.h>

//generic macros
#define APPEND_TRUE 1
#define APPEND_FALSE 0

#define PATTERN_LEN 1024
#define REL_JUMP_LEN 5

#define COPY_OLD 0
#define COPY_NEW 1

#define APPEND_TRUE 1
#define APPEND_FALSE 0

//payload size macros
#define PAYL_NEW_THREAD 41


//byte
typedef unsigned char byte;

//vector
typedef struct {

    byte * vector;
    size_t data_size;
    unsigned long length;

} vector;

//single region in /proc/<pid>/maps
typedef struct {

        // --- read_maps()
        
        //'pathname' field - backing file if present, else region starting address
        char pathname[PATH_MAX];
        char basename[NAME_MAX];
        //'pathname' field of closest previous maps_entry with a present backing file
        char last_pathname[PATH_MAX];
        byte perms;                     //4 (read) + 2 (write) + 1 (exec)
        
        void * start_addr;
        void * end_addr;
        
        //indices for backwards traversal
        unsigned long obj_vector_index;    //index into maps_data.obj_vector
        unsigned long obj_index;           //index into maps_obj.entry_vector
        unsigned long last_pathname_index; //index to closest previous entry with
                                           //a valid backing file

        // --- get_caves()

        //caves present in this region
        vector cave_vector; //cave

} maps_entry;

//regions grouped by backing file/type
typedef struct {

        char pathname[PATH_MAX];
        char basename[NAME_MAX];

        vector entry_vector; //*maps_entry

        //INTERNAL - used to set index in the next maps_entry when building maps_data
        unsigned long next_entry_index;

} maps_obj;

//entire memory map
typedef struct {

        vector obj_vector;   //maps_obj
        vector entry_vector; //maps_entry

        //INTERNAL - stores the index to the closest previous entry with a valid
        //           backing file, for use 
        unsigned long last_pathname_index;

} maps_data;


//pattern to search for
typedef struct {

    maps_entry * search_region;
    byte pattern_bytes[PATTERN_LEN];
    int pattern_len;
    vector offset_vector;

} pattern;

//cave - unused region of memory, typically zero'ed out
typedef struct {

    unsigned int offset;
    int size;

} cave;

//injection metadata
typedef struct {

    maps_entry * target_region;
    unsigned int offset;

    byte * payload;
    unsigned int payload_size;

} raw_injection;

//relative 32bit jump hook
typedef struct {

    maps_entry * from_region;
    uint32_t from_offset; //address of jump instruction

    maps_entry * to_region;
    uint32_t to_offset;

} rel_jump_hook;

//process name to find matching PIDs for
typedef struct {

    char basename[NAME_MAX];
    vector pid_vector; //pid_t

} name_pid;

//information about puppet process
typedef struct {

    pid_t pid;

    void * syscall_addr; //address of a syscall instruction

    struct user_regs_struct saved_state;
    struct user_fpregs_struct saved_float_state;

    struct user_regs_struct new_state;
    struct user_fpregs_struct new_float_state;

} puppet_info;

//information for bootstrapping a new thread via an auto payload
typedef struct {

	maps_entry * thread_func_region;
	maps_entry * setup_region;
	unsigned int thread_func_offset;
	unsigned int setup_offset;
	void * stack_addr; //as returned by create_thread_stack()
	unsigned int stack_size;

} new_thread_setup;

//payload mutations structure
typedef struct {

	unsigned int offset;
	byte * mod;
	int mod_len; //beware of bounds, i trust you!

} mutation;

//symbol address resolution structure
typedef struct {

    void * lib_handle;
    maps_data * host_m_data;
    maps_data * target_m_data;

} sym_resolve;


// --- READING PROCESS MEMORY MAPS ---
//read /proc/<pid>/maps into allocated maps_data object
//returns: 0 - success, -1 - failed to read maps
extern int read_maps(maps_data * m_data, FILE * maps_stream);
//returns: 0 - success, -1 - failed to allocate space
extern int new_maps_data(maps_data * m_data);
//returns: 0 - success, -1 - failed to deallocate maps_data
extern int del_maps_data(maps_data * m_data);


// --- INJECTION ---
//returns: number of caves found on success, -1 - failed to search memory
extern int get_caves(maps_entry * m_entry, int fd_mem, int min_size, cave * first_cave);
//returns: 0 - success, -1 - failed to find big enough cave, -2 fail to search_region
extern int find_cave(maps_entry * m_entry, int fd_mem, int required_size,
	                 cave * matched_cave);
//returns: 0 - success, -1 - failed to inject
extern int raw_inject(raw_injection r_injection_dat, int fd_mem);
//returns: 0 - success, -1 - fail
extern int new_raw_injection(raw_injection * r_injection_dat, maps_entry * target_region,
                             unsigned int offset, char * payload_filename);
//returns: 0 - success, -1 - fail
extern void del_raw_injection(raw_injection * r_injection_dat);

// --- HOOKING ---
//returns: old relative jump offset on success, 0 on fail to hook
extern uint32_t hook_rj(rel_jump_hook rj_hook_dat, int fd_mem);


// --- SEARCHING FOR PATTERNS IN MEMORY ---
//returns: 0 - success, -1 - failed to allocate object
//search_region can be NULL and be set later
extern int new_pattern(pattern * ptn, maps_entry * search_region, byte * bytes_ptn,
                       int bytes_ptn_len);
//returns: 0 - success, -1 - failed to deallocate object
extern int del_pattern(pattern * ptn);
//returns: n - number of patterns, -1 - failed to search for patterns
extern int match_pattern(pattern * ptn, int fd_mem);


// --- ATTACHING TO PROCESS ---
//returns: 0 - success, -1 - failed to attach
extern int puppet_attach(puppet_info p_info);
//returns: 0 - success, -1 - failed to detach
extern int puppet_detach(puppet_info p_info);
//returns: 0 - success, -1 - no syscall found, -2 - failed to perform search
extern int puppet_find_syscall(puppet_info * p_info, maps_data * m_data, int fd_mem);
//returns: 0 - success, -1 - failed to save registers
extern int puppet_save_regs(puppet_info * p_info);
//returns: 0 - success, -1 - failed to write registers
extern int puppet_write_regs(puppet_info * p_info);
//returns: void
extern void puppet_copy_regs(puppet_info * p_info, int mode);

//returns: 0 - success, -1 - failed to execute syscall
extern int arbitrary_syscall(puppet_info * p_info, int fd_mem,
                             unsigned long long * syscall_ret);
//returns: 0 - success, -1 - failed to change region permissions
extern int change_region_perms(puppet_info * p_info, byte perms, int fd_mem,
                               maps_entry * target_region);
//returns: 0 - success, -1 - failed create new thread stack
extern int create_thread_stack(puppet_info * p_info, int fd_mem, void ** stack_addr,
                               unsigned int stack_size);
//returns: 0 - success, -1 - failed to start thread
extern int start_thread(puppet_info * p_info, int fd_mem,
	                    new_thread_setup n_t_setup, int * tid);

// --- FINDING PIDs BY NAME ---
//returns: 0 - success, -1 - failed to allocate object
extern int new_name_pid(name_pid * n_pid, char * name);
//returns: 0 - success, -1 - failed to deallocate object
extern int del_name_pid(name_pid * n_pid);
//returns: number of PIDs with matching name found, -1 on error
extern int pid_by_name(name_pid * n_pid, pid_t * first_pid);
//returns: 0 - success, -1 - failed to read process name
extern int name_by_pid(pid_t pid, char * name);

// --- MISCELLANEOUS UTILITIES ---
//convert bytes to hex string
// inp's length = inp_len
// out's length = inp_len * 2
// 0x omitted from beginning of out's string
extern void bytes_to_hex(byte * inp, int inp_len, char * out);
//returns: 0 - success, -1 - couldn't open memory file(s)
extern int open_memory(pid_t pid, FILE ** fd_maps, int * fd_mem);
//returns: 0 - success, -1 - fail
extern int sig_stop(pid_t pid);
//returns: 0 - success, -1 - fail
extern int sig_cont(pid_t pid);


// --- PAYLOAD MUTATIONS
//return 0 on success, -1 on fail
extern int new_mutation(mutation * m, size_t buf_size);
extern void del_mutation(mutation * m);
extern int apply_mutation(byte * payload_buffer, mutation * m);


// --- SYMBOL RESOLVING
//returns: 0 - success, -1 - fail
extern int open_lib(char * lib_path, sym_resolve * s_resolve);
//returns: void
extern void close_lib(sym_resolve * s_resolve);
//returns: symbol address - success, NULL - fail
extern void * get_symbol_addr(char * symbol, sym_resolve s_resolve);
//returns: 0 - success, -1 - no match found, -2 - fail
extern int get_region_by_addr(void * addr, maps_entry ** matched_region,
                              unsigned int * offset, maps_data * m_data);
//returns: 0 - success, -1 - no match found, -2 - fail
extern int get_obj_by_pathname(char * pathname, maps_data * m_data,
                               maps_obj ** matched_obj);
//returns: 0 - success, -1 - no match found, -2 - fail
extern int get_obj_by_basename(char * basename, maps_data * m_data,
                               maps_obj ** matched_obj);
//returns: 0 - success, -1 - no match found, -2 - fail
extern int resolve_symbol(char * symbol, sym_resolve s_resolve,
                          maps_entry ** matched_region, unsigned int * matched_offset);



// --- VECTOR OPERATIONS ---
//returns: 0 - success, -1 - fail
int new_vector(vector * v, size_t data_size);
//returns: 0 - success, -1 - fail
int del_vector(vector * v);
//returns: 0 - success, -1 - fail
int vector_add(vector * v, unsigned long pos, byte * data, unsigned short append);
//returns: 0 - success, -1 - fail
int vector_rmv(vector * v, unsigned long pos);
//returns: 0 - success, -1 - fail
extern int vector_get(vector * v, unsigned long pos, byte * data);
//returns: 0 - success, -1 - fail
extern int vector_get_ref(vector * v, unsigned long pos, byte ** data);


// --- MEMORY OPERATIONS ---
//returns: 0 - success, -1 - fail
extern int read_mem(int fd_mem, void * addr, byte * read_buf, int len);
//returns: 0 - success, -1 - fail
extern int write_mem(int fd_mem, void * addr, byte * write_buf, int len);

#ifdef __cplusplus
}
#endif

#endif
