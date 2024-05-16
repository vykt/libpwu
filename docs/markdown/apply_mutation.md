## apply\_mutation()

```c
int apply_mutations(byte * payload_buffer, mutation * m);
```

### description
`apply_mutation()` function takes a mutation structure and applies it to an in-memory payload pointed at by payload buffer.

### parameters
- `*payload_buffer` : pointer to a payload buffer.
- `*m`              : mutation structure to apply to the payload.

### return value
`0` on success, `-1` on fail.
