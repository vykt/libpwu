## new\_mutation()

```c
int new_mutation(mutation * m, size_t buf_size);
```

### description
`new_mutation()` initialises a structure used to apply mutations to a payload. The individual fields of m should be set manually. A single mutation structure should be sufficient to perform every necessary mutation on a payload.

### parameters
- `*m`        : pointer to the mutation to be initialised.
- `buf_size`  : how many bytes to allocate for the buffer that will hold the mutation.

### return value
`0` on success, `-1` on fail.
