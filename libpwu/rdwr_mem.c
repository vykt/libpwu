#include <stdio.h>
#include <unistd.h>

#include "libpwu.h"
#include "rdwr_mem.h"


//read memory at address into read_buf
int read_mem(int fd_mem, void * addr, byte * read_buf, int len) {

	long page_size;
	ssize_t rdwr_ret;
	off_t off_ret;

	ssize_t rd_done = 0;

	//get page size
	page_size = sysconf(_SC_PAGESIZE);
	if (page_size < 0) return -1;

	//seek to address
	off_ret = lseek(fd_mem, (off_t) addr, SEEK_SET);
	if (off_ret == -1) return -1;

	//read page_size bytes repeatedly until done
	do {

		//read into buffer
		rdwr_ret = read(fd_mem, read_buf, page_size);
		//if error or EOF before reading len bytes
		if (rdwr_ret == -1 || (rdwr_ret == 0 && rd_done < len)) return -1;
		rd_done += rdwr_ret;

	} while (rd_done < len);

	return 0;
}

//write memory at address from write_buf
int write_mem(int fd_mem, void * addr, byte * write_buf, int len) {

	long page_size;
	ssize_t rdwr_ret;
	size_t rdwr_size;
	size_t rdwr_total = 0;
	off_t off_ret;

	//get page size
	page_size = sysconf(_SC_PAGESIZE);
	if (page_size < 0) return -1;

	//seek to address
	off_ret = lseek(fd_mem, (off_t) addr, SEEK_SET);
	if (off_ret == -1) return -1;

	//write page_size bytes repeatedly until done
	do {

		//get size to write
		if ((rdwr_total + page_size) > len) {
			rdwr_size = len - rdwr_total;
		} else {
			rdwr_size = page_size;
		}

		//write from buffer
		rdwr_ret = write(fd_mem, write_buf, rdwr_size);
		//if error or EOF before writing len bytes
		if (rdwr_ret == -1 || (rdwr_ret == 0 && rdwr_ret < len)) return -1;
		rdwr_total += rdwr_ret;

	} while (rdwr_total < len);
	
	return 0;
}
