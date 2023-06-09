#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "libpwu.h"
#include "vector.h"


//Set element's data
int vector_set(vector * v, unsigned long pos, byte * data) {

	//Check for NULL
	if (v == NULL || data == NULL) {
		return -1;
	}

	//Check if asking to set out of bounds
	if (pos >= v->length) {
		return -1;
	}

	//Copy data to vector entry
	memcpy(v->vector + (pos * v->data_size), data, v->data_size);
	
	return 0;
}


//Add element
int vector_add(vector * v, unsigned long pos, byte * data, unsigned short append) {

	//Check for NULL
	if (v == NULL || v->data_size == 0) {
		return -1;
	}

	//Check if max capacity reached
	if (v->length == UINT64_MAX) {
		return -1;
	}

	//Check if asking for data out of bounds
	if (pos > v->length && append == APPEND_FALSE) {
		return -1;
	}

	//Check for append
	if (append == APPEND_TRUE) {
		pos = v->length;
	}

	v->length = v->length + 1;
	v->vector = realloc(v->vector, (size_t) (v->length * v->data_size));
	if (v->vector == NULL) {
		return -1;
	}

	//Move vector entries	
	if (pos != v->length - 1) {	
		memmove(
			v->vector + ((pos + 1) * v->data_size),
			v->vector + (pos * v->data_size),
			((v->length - 1 - pos) * v->data_size)
		);
	}

	//Copy data over if necessary
	if (data != NULL) {
		memcpy(v->vector + (pos * v->data_size), data, v->data_size);
	}

	return 0;
}


//Remove element
int vector_rmv(vector * v, unsigned long pos) {

	//Check for NULL
	if (v == NULL) {
		return -1;
	}

	//Check if vector is empty
	if (v->length == 0) {
		return -1;
	}

	//Check if asking for data out of bounds
	if (pos >= v->length) {
		return -1;
	}

	if (pos < v->length - 1) {
		memmove(
			v->vector + (pos * v->data_size),
			v->vector + ((pos + 1) * v->data_size),
			(v->length - 1 - pos) * v->data_size
		);
	} //End if

	v->length = v->length - 1;
	v->vector = realloc(v->vector, v->length * v->data_size);

	return 0;
}


//Get element
int vector_get(vector * v, unsigned long pos, byte * data) {

	//Check for NULL
	if (v == NULL) {
		return -1;
	}

	//Check if vector is empty
	if (v->length == 0) {
		return -1;
	}

	//Check if asking for data out of bounds
	if (pos >= v->length) {
		return -1;
	}

	//Clear buffer;
	memset(data, 0, v->data_size);

	//Get value
	memcpy(data, v->vector + (pos * v->data_size), v->data_size);

	return 0;
}


//Get element by reference
int vector_get_ref(vector * v, unsigned long pos, byte ** data) {
	
	//Check for NULL
	if (v == NULL) {
		return -1;
	}

	//Check if vector is empty
	if (v->length == 0) {
		return -1;
	}

	//Check if asking for data out of bounds
	if (pos >= v->length) {
		return -1;
	}

	*data = v->vector + (pos * v->data_size);

	return 0;
}


//Get index of element in vector by value
int vector_get_pos_by_dat(vector * v, byte * data, unsigned long * pos) {

	//Check for NULL
	if (v == NULL || data == NULL) {
		return -1;
	}

	int ret;
	byte * data_cmp;

	//Try get position
	for (unsigned long i = 0; i < v->length; ++i) {
		ret = vector_get_ref(v, i, &data_cmp);
		if (ret != 0) return ret;

		//Here, compare only the sockaddr_in6 part, not the last_ping
		ret = memcmp(data, data_cmp, sizeof((struct sockaddr_in6 *) data));
		if (ret) continue;
		*pos = i;
		return 0;
	}

	return -1;
}


//Move element
int vector_mov(vector * v, unsigned long pos, unsigned long pos_new) {

	if (pos == pos_new) return 0;

	int ret;

	byte * data = malloc(v->data_size);
	if (data == NULL) return -1;

	//Get data of both indexes
	ret = vector_get(v, pos, data);
	if (ret != 0) { free(data); return ret; }

	//If moving towards beginning
	if (pos > pos_new) {
		memmove(v->vector+((pos_new + 1) * v->data_size),
				v->vector+(pos_new * v->data_size),
				v->data_size * (pos - pos_new));
	//else moving towards end
	} else {
		memmove(v->vector+(pos * v->data_size),
				v->vector+((pos + 1) * v->data_size),
			    v->data_size * (pos_new - pos));
	} //End if moving towards beginning/end

	ret = vector_set(v, pos_new, data);
	if (ret != 0) { free(data); return ret; }

	free(data);
	return 0;
}


//Start vector
int new_vector(vector * v, size_t data_size) {

	//Check NULL
	if (v == NULL || data_size == 0) {
		return -1;
	}

	v->vector = malloc(0);
	v->data_size = data_size;
	v->length = 0;

	return 0;
}


//End vector, free memory
int del_vector(vector * v) {

	//Check NULL
	if (v == NULL) {
		return -1;
	}

	free(v->vector);

	return 0;
}
