#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/limits.h>

#include "libpwu.h"
#include "process_maps.h"
#include "vector.h"

/*
 *	Intended use:
 *
 *	1) call new_maps_data()
 *	2) call read_maps()
 *	3) do whatever you want with the data
 *	4) call del_maps_data()
 */


//populate objects
int build_obj_vector(maps_data * m_data) {

	int ret;
	int pos;
	maps_entry * temp_m_entry;
	maps_obj temp_m_obj;
	maps_obj * temp_m_obj_ref;

	//for every maps entry
	for (int i = 0; i < m_data->entry_vector.length; ++i) {

		ret = vector_get_ref(&m_data->entry_vector, i, (byte **) &temp_m_entry);
		if (ret == -1) return -1;

		//get the object entry where the backing object for this entry occurs
		pos = entry_path_match(*temp_m_entry, *m_data);
		
		//if it doesn't occur yet
		if (pos == -1) {

			//create new maps object
			ret = new_maps_obj(&temp_m_obj, temp_m_entry->pathname, 
                               temp_m_entry->basename);
			if (ret == -1) return -1;

			//add entry to this map object
			ret = vector_add(&temp_m_obj.entry_vector, 0, (byte *) &temp_m_entry,
					         APPEND_TRUE);
			if (ret == -1) return -1;

            //set the obj_index value for this maps_entry
            temp_m_entry->obj_index = temp_m_obj.next_entry_index;

            //increment next_entry_index for this maps_obj
            temp_m_obj.next_entry_index += 1;

			//add map object to map data passed by caller
			ret = vector_add(&m_data->obj_vector, 0, (byte *) &temp_m_obj,
					         APPEND_TRUE);
			if (ret == -1) return -1;

            //set the maps_data.obj_vector index for this maps_entry
            temp_m_entry->obj_vector_index = m_data->obj_vector.length - 1;

		//if the name matches an object entry already present
		} else {
	
			//fetch existing map object
			ret = vector_get_ref(&m_data->obj_vector, pos, (byte **) &temp_m_obj_ref);
			if (ret == -1) return -1;

            //set the obj_index and maps_data.obj_vector values for this maps_entry
            temp_m_entry->obj_index = temp_m_obj_ref->next_entry_index;
            temp_m_entry->obj_vector_index = pos;

            //increment next_entry_index for this maps_obj
            temp_m_obj_ref->next_entry_index += 1;

            //associate this maps_entry with this maps_obj
			ret = vector_add(&temp_m_obj_ref->entry_vector, 0, (byte *) &temp_m_entry, 
					         APPEND_TRUE);
			if (ret == -1) return -1;

		}

	}// for every maps entry

	return 0;
}


//read /proc/<pid>/maps file
int read_maps(maps_data * m_data, FILE * maps_stream) {

	int ret;
    int entry_index;
	char line[LINE_LEN];
    char * basename_ptr;
	maps_entry temp_m_entry;

	//while there are entries in /proc/<pid>/maps left to process
	entry_index = 0;
    while(!get_maps_line(line, maps_stream)) {

		//initiate cave vector, zero out all other initial values
		ret = new_maps_entry(&temp_m_entry);
		if (ret == -1) return -1;

		//store address range in temporary entry
		ret = get_addr_range(line, &temp_m_entry.start_addr, &temp_m_entry.end_addr);
		if (ret == -1) return -1; //error reading maps file
		
		//store permissions and name of backing file in temporary entry
		ret = get_perms_name(line, &temp_m_entry.perms, temp_m_entry.pathname);
		
		//if there is no pathname for a given entry, set it to <NO_PATHNAME>
		if (ret == -1) {
            strcpy(temp_m_entry.pathname, "<NO_PATHNAME>");
            temp_m_entry.last_pathname_index = m_data->last_pathname_index;
        //if there is a pathname, then update maps_data's last_pathname_cache with it
        } else {
            m_data->last_pathname_index = entry_index;
            temp_m_entry.last_pathname_index = m_data->last_pathname_index;
        }

        //set the basename
        basename_ptr = strrchr(temp_m_entry.pathname, (int) '/') + 1;
        if (basename_ptr == (char *) 1) {
            strcpy(temp_m_entry.basename, temp_m_entry.pathname);
        } else {
            strcpy(temp_m_entry.basename, basename_ptr);
        }

		//add temporary entry to entry vector
		ret = vector_add(&m_data->entry_vector, 0, (byte *) &temp_m_entry,
						 APPEND_TRUE);
		if (ret == -1) return -1;

        //increment index
        entry_index++;
	}

	//now populate
	ret = build_obj_vector(m_data);

	//if out of loop, all entries are read successfully
	return 0;
}


//search through m_obj_arr for a maps_obj with matching name
//return: success: index, fail: -1
int entry_path_match(maps_entry temp_m_entry, maps_data m_data) {

	int ret;
	maps_obj m_obj;

	//for every maps_obj in m_data
	for (int i = 0; i < m_data.obj_vector.length; ++i) {

		ret = vector_get(&m_data.obj_vector, i, (byte *) &m_obj);
		if (ret == -1) return -1; //unable to fetch vector entry

		ret = strcmp(temp_m_entry.pathname, m_obj.pathname);
		if (ret == 0) {
			return (int) i; //return index into obj_vector in case of match
		}
	}
	//if no match found
	return -1;
}


//initialise new maps_data
int new_maps_data(maps_data * m_data) {

	int ret;
	ret = new_vector(&m_data->obj_vector, sizeof(maps_obj));
	if (ret == -1) return -1;
	ret = new_vector(&m_data->entry_vector, sizeof(maps_entry));
	return ret; //return 0 on success, -1 on fail
}


//detele maps_data, free all allocated memory inside
int del_maps_data(maps_data * m_data) {

	int ret;
	maps_obj m_obj;

	//for every maps_data object, delete it and its entry members
	for (int i = 0; i < m_data->obj_vector.length; ++i) {

		ret = vector_get(&m_data->obj_vector, i, (byte *) &m_obj);
		if (ret == -1) return -1; //unable to fetch vector entry

		ret = del_maps_obj(&m_obj);
		if (ret == -1) return -1; //attempting to free non-existant vector

	}

	ret = del_vector(&m_data->obj_vector);
	if (ret == -1) return -1;
	ret = del_vector(&m_data->entry_vector);
	return ret; //return 0 on success, -1 on fail
}


//get entry from maps file
int get_maps_line(char line[LINE_LEN], FILE * maps_stream) {

	//first, zero out line
	memset(line, '\0', LINE_LEN);

	if (fgets(line, LINE_LEN, maps_stream) == NULL) {
		return -1;
	}
	return 0;
}


//initialise new maps_obj
int new_maps_obj(maps_obj * m_obj, char pathname[PATH_MAX], char basename[NAME_MAX]) {

	int ret;

	//first, zero out the name fields
	memset(m_obj->pathname, '\0', PATH_MAX);
    memset(m_obj->basename, '\0', NAME_MAX);

	//now copy names in
	strcpy(m_obj->pathname, pathname);
    strcpy(m_obj->basename, basename);

    //initialise next entry index
    m_obj->next_entry_index = 0;

	//now, initialise vector member
	ret = new_vector(&m_obj->entry_vector, sizeof(maps_entry *));
	return ret; //return 0 on success, -1 on fail
}


//delete maps_obj
int del_maps_obj(maps_obj * m_obj) {

	int ret;
	maps_entry * m_entry;

	//for every entry vector member
	for (int i = 0; i < m_obj->entry_vector.length; ++i) {
		//get the entry
		ret = vector_get(&m_obj->entry_vector, i, (byte *) &m_entry);
		if (ret == -1) return -1;
		//free its cave vector
		ret = del_maps_entry(m_entry);
		if (ret == -1) return -1;
	}


	ret = del_vector(&m_obj->entry_vector);
	return ret; //return 0 on success, -1 on fail
}


//initialise new maps_entry
int new_maps_entry(maps_entry * m_entry) {

	int ret;

	memset(m_entry->pathname, 0, PATH_MAX);
	memset(m_entry->basename, 0, NAME_MAX);
    m_entry->perms = 0;
	m_entry->start_addr = NULL;
	m_entry->end_addr = NULL;

	ret = new_vector(&m_entry->cave_vector, sizeof(cave));
	return ret;
}


//delete maps_entry, free all allocated memory inside
int del_maps_entry(maps_entry * m_entry) {

	int ret;
	ret = del_vector(&m_entry->cave_vector);
	return ret;
}


//get the address range values from a line in /proc/<pid>/maps
int get_addr_range(char line[LINE_LEN], void ** start_addr, void ** end_addr) {

	int next = 0;
	int start_count = 0;
	int end_count = 0;

	char buf_start[LINE_LEN / 2] = {0};
	char buf_end[LINE_LEN / 2] = {0};

	//for every character of line
	for (int i = 0; i < LINE_LEN; ++i) {

		if (line[i] == '-') {next = 1; continue;}
		if (line[i] == ' ') break;

		if(!next) {
			buf_start[start_count] = line[i];
			++start_count;
		} else {
			buf_end[end_count] = line[i];
			++end_count;
		}

	} //end for every character of line
	
	*start_addr = (void *) strtol(buf_start, NULL, 16);
	*end_addr = (void *) strtol(buf_end, NULL, 16);
	if (*start_addr == 0 || *end_addr == 0) {
		return -1; //convertion failed
	}
	return 0;  //convertion successful
}


//get name for line in /proc/<pid>/maps
int get_perms_name(char line[LINE_LEN], byte * perms, char name[PATH_MAX]) {

	//zero out name first
	memset(name, '\0', PATH_MAX);
	*perms = 0;

	//get to end of line
	int i = 0;
	int j = 0;
	size_t name_len;
	int column_count = 0;

	while (line[i] != '\0' && i < LINE_LEN-1) {
		
		//if at permissions
		/*
		 *	Permissions are 1 - read, 2 - write, 4 - exec
		 *	Yes, that's the reverse of the filesystem perms. 
		 *	This is the format mprotect() uses.
		 */
		if(column_count == 1) {
			if (line[i] == 'r') *perms = *perms + 1;
			if (line[i+1] == 'w') *perms = *perms + 2;
			if (line[i+2] == 'x') *perms = *perms + 4;
			i+=4;
		}

		//if reached the void between offset and name
		if (column_count == 5) {
			if (line[i] == '\n') break;
			if (line[i] == ' ') {
				++i;
				continue;
			}
			name[j] = line[i];
			++j;
		}

		if (line[i] == ' ') {++column_count;}
		++i;
	} //end get to end of line
	
	//if name exists
	if (j) {
		name_len = strlen(name);
		name[name_len] = '\0';
		return 0;
	}
	//if no name exists
	return -1;
}
