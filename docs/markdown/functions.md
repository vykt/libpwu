# help

## description

This page serves as a catalog of libpwu manpages for structures and functions. All manpages for each function are prefixed with 'libpwu\_'

# catalog


### structures:
```
structs : structures of libpwu
```


### hooks:
```
hook_rj : hook relative jump
```

### injection:
```
get_caves         : find caves inside a specific memory region
find_cave         : find cave that meets size requirement

raw_inject        : raw inject an in-memory payload into memory region
new_raw_injection : initialise new raw injection
del_raw_injection : delete raw injection
```

### mutation:
```
new_mutation   : initialise a mutation structure
del_mutation   : delete a mutation structure
apply_mutation : apply a mutation to an in-memory payload
```


### name to PID:
```
name_by_pid  : get process name from PID
pid_by_name  : fill a vector with every name match of PID
new_name_pid : initialise new name_pid object for matching name to PID
del_name_pid : delete name_pid object
```


### pattern scanning:
```
match_pattern : match each instance of pattern
new_pattern   : initialise a new pattern object
del_pattern   : delete a pattern object
```


### process maps:
```
read_maps     : read /proc/<pid>/maps file

new_maps_data : initialise new maps_data object storing contents of proc_maps
del_maps_data : delete maps_data object
```


### puppeting:
```
puppet_attach       : clean ptrace attach
puppet_detach       : clean ptrace detach

puppet_find_syscall : locate executable syscall instruction
puppet_save_regs    : save state of suspended task
puppet_write_regs   : write to state of suspended task
puppter_copy_regs   : copy state of suspended task

arbitrary_syscall   : execute a syscall in the task

change_region_perms : change permissions of a memory region
create_thread_stack : allocate a stack for a new thread in the target process

start_thread        : start thread in the task project
```


### read & write process memory:
```
read_mem  : read process memory
write_mem : write process memory
```


### symbol resolution
```
open_lib            : open shared object in own process
close_lib           : close shared object in own process

get_symbol_addr     : get address of symbol

get_region_by_addr  : find which memory region an address resides in
get_obj_by_pathname : find memory object that matches a pathname
get_obj_by_basename : find memory object that matches a basename
```


### util
```
bytes_to_hex : convert a byte string to its hexadecimal ASCII string representation
open_memory  : open maps stream and mem file descriptor for target process
sig_stop     : send SIGSTOP to attached process
sig_cont     : send SIGCONT to attached process
```


### vector operations
```
vector_add     : add entry to vector
vector_rmv     : remove entry from vector

vector_get     : get vector entry by value 
vector_get_ref : get vector entry by reference (pointer)

new_vector     : initialise new vector
del_vector     : delete vector
```
