#include <stdlib.h>
#include <string.h>

#include "libpwu.h"
#include "mutate.h"
#include "vector.h"


//initialise mutation object
int new_mutation(mutation * m, size_t buf_size) {

    m->mod = malloc(buf_size);
    if (m->mod == NULL) return -1;

    m->offset = 0;
    m->mod_len = 0;

    return 0;
}


//destroy mutation object
void del_mutation(mutation * m) {

    free(m->mod);
}



//apply mutation to payload
int apply_mutation(byte * payload_buffer, mutation * m) {

    memcpy(payload_buffer + m->offset, m->mod, m->mod_len);

	return 0;
}
