# data structures

## vector

```c
typedef struct {
        
        byte * vector;
        size_t data_size;
        unsigned long length;

} vector;
```

### description
`vector` is this library's vector implementation.

### elements
- `vector`    : pointer to heap allocation holding the vector.
- `data_size` : size of each element.
- `length`    : number of elements in the vector.

### functions
- `vector_get()`
- `vector_get_ref()`

<br>

## maps\_entry 

```c
typedef struct {

        //read_maps()
        char pathname[PATH_MAX];
        char basename[NAME_MAX];
        byte perms;
        void * start_addr;
        void * end_addr;
        unsigned long obj_vector_index;
        unsinged long obj_index;
        unsigned long last_pathname_index;

        //get_caves()
        vector cave_vector; //cave

} maps_entry;
```

### description
`maps_entry` represents a single line in a `/proc/<pid>/maps` file for a single process, sorted into the desired components. Data specific to each segment is also stored here. Created automatically.

### elements
- `pathname`            : name of the backing file for this segment. refer to `proc(5)`.
- `basename`            : basename of backing file for this segment. refer to `proc(5)`.
- `last_pathname`       : backing file of the closest previous segment with one.
- `perms`               : permissions for the region, see `mprotect(2)` for format.
- `start_addr`          : address of the start of this segment in `/proc/<pid>/mem`.
- `end_addr`            : address of the end of this segment in `/proc\<pid\>/mem`.
- `object_vector_index` : index into `maps_data.obj_vector` for this entry.
- `obj_index`           : index into `maps_obj.entry_vector` for this entry's `maps_obj`.
- `last_pathname_index` : index into `maps_data.entry_vector` for closest previous entry with a valid backing file.
- `cave_vector`         : vector of `cave` struct.

### functions
- `get_caves()`

<br>

## maps\_obj

```c
typedef struct {

        char pathname[PATH_MAX];
        char basename[NAME_MAX];
        vector entry_vector; //*maps_entry

} maps_obj;
```

### description
`maps_obj` is a 'backing file'/pathname based view of `map_entry` structures. It holds the backing file and every segment that belongs to it. See `proc(5)`. Created automatically.

### elements
- `name`         : name of the backing file for this object.
- `basename`     : basename of backing file for this object.
- `entry_vector` : vector of pointers to `maps_entry` structures belonging to this backing file.

### functions
none.

<br>

## maps\_data

```c
typedef struct {

        vector obj_vector; //maps_obj
        vector entry_vector; //maps_entry

} maps_data;

```

### description
`maps_data` is the <b>overarching</b> data structure representing the entire `/proc/<pid>/maps` file for a single process. Requires initialisation. Filled by `read_maps()`.

### elements
- `obj_vector`   : vector storing a backing file oriented view of segments.
- `entry_vector` : vector storing segments as they appear in `/proc/<pid>/maps`.

### functions
- `new_maps_data()`
- `del_maps_data()`
- `read_maps()`

<br>

## pattern

```c
typedef struct {

    maps_entry * search_region;
    byte pattern_bytes[PATTERN_LEN];
    int pattern_len;
    vector offset_vector;

} pattern;
```

### description
`pattern` contains members related to performing a byte pattern search on a memory segment. Requires initialisation.

### elements
- `search_region`   : segment to carry out the search on.
- `pattern_bytes`   : pattern of bytes to search for.
- `pattern_len`     : length of the pattern of bytes to search for.
- `offset_vector`   : vector of offsets at which the pattern occurs (first byte).

### functions
- `new_pattern()`
- `del_pattern()`
- `match_pattern()`

<br>

## cave

```c
typedef struct {

    unsigned int offset;
    int size;

} cave;
```

### description
`cave` stores data about areas of unused memory where payloads may be injected. Created automatically inside `maps_entry` by `get_caves()`.

### elements
- `offset` : offset at which the cave begins (first byte).
- `size`   : size of the cave, in bytes.

### functions
- `get_caves()`

<br>

## raw\_injection

```c
typedef struct {

    maps_entry * target_region;
    unsigned int offset;

    byte * payload;
    unsigned int payload_size;

} raw_injection;
```

### description
`raw_injection` stores data for injecting a payload at an arbitrary offset inside a region. Requires initialisation.

### elements
- `target_region` : `maps_entry` segment where the injection will take place.
- `offset`        : offset at which to begin injection (first byte).
- `payload`       : pointer to heap allocated space holding the payload.
- `payload_size`  : size of the payload on the heap in bytes.

### functions
- `new_raw_injection()`
- `del_raw_injection()`
- `raw_inject()`

<br>

## rel\_jump\_hook

```c
typedef struct {

    maps_entry * from_region;
    uint32_t from_offset; //address of jump instruction

    maps_entry * to_region;
    uint32_t to_offset;

} rel_jump_hook;
```

### description
`rel_jump_hook` stores data for hooking an existing 4 byte relative jump and changing the offset to jump to another location. Set manually.

### elements
- `from_region` : `maps_entry` segment where the target relative jump is located.
- `from_offset` : offset at which the relative jump begins (first byte).
- `to_region`   : `maps_entry` segment where the target relative jump will now jump to.
- `to_offset`   : offset to which the target relative jump will now jump to inside the `to_region` segment.

### functions
- `hook_rj()`

<br>

## name\_pid

```c
typedef struct {

    char name[NAME_MAX];
    vector pid_vector; //pid_t

} name_pid;

```

### description
`name_pid` stores the name of a target process and a vector of all processes that match this name. `pid_vector` is populated by `pid_by_name()`. Requires initialisation.

### elements
- `name`       : name of the target process
- `pid_vector` : vector of process IDs that match `name`.

### functions
- `new_name_pid()`
- `del_name_pid()`
- `pid_by_name()`

<br>

## puppet\_info

```c
typedef struct {

    pid_t pid;

    void * syscall_addr;

    struct user_regs_struct saved_state;
    struct user_fpregs_struct saved_float_state;

    struct user_regs_struct new_state;
    struct user_fpregs_struct new_float_state;

} puppet_info;

```

### description
`puppet_info` stores data required to attach to a process and change the permissions of its segments. Set `pid` manually, the rest is for internal use.

### elements
- `pid`               : target process ID.
- `*syscall_addr`     : syscall instruction address in executable memory of puppet.
- `saved_state`       : registers at time of puppet.
- `saved_float_state` : floating point registers at time of puppet.
- `new_state`         : registers for `mprotect` syscall.
- `new_float_state`   : floating point registers for `mprotect` call.

### functions
- `puppet_attach()`
- `puppet_detach()`
- `puppet_find_syscall()`
- `puppet_save_regs()`
- `puppet_write_regs()`
- `puppet_copy_regs()`
- `change_region_perms()`

<br>

## new\_thread\_setup

```c
typedef struct {

    maps_entry * thread_func_region;
    maps_entry * setup_region;
    unsigned int thread_func_offset;
    unsigned int setup_offset;
    void * stack_addr;
    unsigned int stack_size;

} new_thread_setup;

```

### description
`new_thread_setup` contains data needed to create a new thread inside the target process. `stack_addr` must be initialised with `create_thread_stack()`. `thread_func_region` and `thread_func_offset` must be set manually.

### elements
- `thread_func_region` : segment where the thread function resides, set manually.
- `setup_region`       : segment where the setup payload will be injected, set manually.
- `thread_func_offset` : offset for the thread function in its segment, set manually.
- `setup_offset`       : offset to inject at inside the setup segment, set manually.
- `stack_addr`         : new thread stack, set by `create_thread_stack()` 
- `stack_size`         : stack size, set manually prior to `create_thread_stack()`

### functions
- `create_thread_stack()`
- `start_thread()`

<br>

## mutation

```c
typedef struct {

    unsigned int offset;
    byte * mod;
    int mod_len;

} mutation;

```

### description
`mutation` is a mutation blueprint applied to a payload by the `apply_mutation()` function.

### elements
- `offset`  : offset into the payload at which to begin the mutation, set manually.
- `mod`     : buffer holding the mutation, allocated by initialiser, set manually;
- `mod_len` : the real length of the mutation stored in `mod[32]`, set manually.

### functions
- `new_mutation()`
- `del_mutation()`
- `apply_mutation()`

<br>

## sym\_resolve

```c
typedef struct {

    void * lib_handle;
    maps_data * host_m_data;
    maps_data * target_m_data;

} sym_resolve;

```

### description
`sym_resolve` stores data for resolving shared object symbols in the target process.

### elements
- `*lib_handle`    : shared object handle returned by `open_lib()`.
- `*host_m_data`   : own process process maps, populate manually.
- `*target_m_data` : target process maps, populate manually.

### functions
- `open_lib()`
- `close_lib()`
- `get_symbol_addr()`
- `resolve_symbol()`
