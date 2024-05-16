#ifndef MUTATE_H
#define MUTATE_H

#include "libpwu.h"
#include "vector.h"

//external
int new_mutation(mutation * m, size_t buf_size);
void del_mutation(mutation * m);
int apply_mutation(byte * payload_buffer, mutation * m);

#endif
