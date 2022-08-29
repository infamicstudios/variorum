// Copyright 2020-2021 Lawrence Livermore National Security, LLC and other
// // Variorum Project Developers. See the top-level LICENSE file for details.
// //
// // SPDX-License-Identifier: MIT

/* msr_fake.c
 * Compile:
 * gcc -Wextra -Werror -shared -fPIC -ldl -o msr_fake.so msr_fake.c
 *
 * Use:
 * LD_PRELOAD=./msr_fake.so ~/copperopolis/build/examples/variorum-print-frequency-example
 */


#define _GNU_SOURCE

#include <dlfcn.h>	// dlopen
#include <stdio.h>	// fprintf, sscanf
#include <stdarg.h>	// va_start(), va_end()
#include <sys/types.h>	// open(), stat()
#include <sys/stat.h>	// open(), stat()
#include <fcntl.h>	// open()
#include <sys/ioctl.h>	// ioctl()
#include <unistd.h>	// close(), pread(), pwrite(), stat()
#include <stdint.h>	// uint64_t and friends
#include <inttypes.h>	// PRIx64 and friends
#include <stdbool.h>
#include <string.h>
#include "msr_core.h"
//#include "063f_msr_samples.h"
#include <errno.h>

// This can be made arbitrarily more complicated.
enum {
	ALLOWLIST_FD=4097,
	BATCH_FD=4098,
	STOCK_FD=4099,
	SAFE_FD=5000
};

enum {
	ALLOWLIST_IDX=0,
	BATCH_IDX,
	STOCK_IDX,
	SAFE_IDX,
	NUM_FILETYPES,
};

struct file{
	int fd;
	bool isopen;
	mode_t init_mode;
} files[NUM_FILETYPES] = {
	{ ALLOWLIST_FD, false, S_IRUSR | S_IWUSR },
	{ BATCH_FD,     false, S_IRUSR | S_IWUSR },
	{ STOCK_FD,     false, S_IRUSR | S_IWUSR },
	{ SAFE_FD,      false, S_IRUSR | S_IWUSR }
};

struct msr 
{
        uint64_t value;
        bool     valid;
        bool     isAllowed;
        uint64_t writeMask;
};

// A struct containing saved values of all valid MSR's
struct msr MSRDAT[]={ 
        #include "msrdat.h"    
};

// Need pread, pwrite, ioctl, open, close, stat.

//
// typedefs
//
// int open(const char *pathname, int flags);
// int open(const char *pathname, int flags, mode_t mode);
typedef int (*real_open_t)( const char *pathname, int flags, ... );
// int close(int fd);
typedef int (*real_close_t)( int fd );
// ssize_t pread(int fd, void *buf, size_t count, off_t offset);
typedef int (*real_pread_t)( int fd, void *buf, size_t count, off_t offset );
// ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
typedef int (*real_pwrite_t)( int fd, const void *buf, size_t count, off_t offset );
// int ioctl(int fd, unsigned long request, ...);
typedef int (*real_ioctl_t)( int fd, unsigned long request, ... );
// int stat(const char *pathname, struct stat *statbuf);
typedef int (*real_stat_t)( const char *pathname, struct stat *statbuf );

//
// Pass-through calls
// Template provided by:
// http://www.goldsborough.me/c/low-level/kernel/2016/08/29/16-48-53-the_-ld_preload-_trick/
//

int
real_open( const char *pathname, int flags, mode_t mode ){
	return ((real_open_t)dlsym(RTLD_NEXT, "open"))( pathname, flags, mode );
}

int
real_close( int fd ){
	return ((real_close_t)dlsym(RTLD_NEXT, "close"))( fd );
}

ssize_t
real_pread( int fd, void *buf, size_t count, off_t offset ){
	return ((real_pread_t)dlsym(RTLD_NEXT, "pread"))( fd, buf, count, offset );
}

ssize_t
real_pwrite( int fd, const void *buf, size_t count, off_t offset ){
	return ((real_pwrite_t)dlsym(RTLD_NEXT, "pwrite"))( fd, buf, count, offset );
}

int
real_ioctl( int fd, unsigned long request, char *arg_p ){
	return ((real_ioctl_t)dlsym(RTLD_NEXT, "ioctl"))( fd, request, arg_p );
}

int
real_stat(const char *pathname, struct stat *statbuf){
	return ((real_stat_t)dlsym(RTLD_NEXT, "stat"))( pathname, statbuf );
}

//
// Utility Methods
//


bool
isMSRSafeFile(const char *pathname) {
   
   int cpunum;
   int res = sscanf(pathname, "/dev/cpu/%d/msr", &cpunum);

   if (res != 0) {
     return true;   
   }
   else {
       return false;
   }
}

//
// Interceptors
//
int
open(const char *pathname, int flags, ... ){

	mode_t mode=0;

	// Stanza taken from https://musl.libc.org
	if ((flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE) {
                va_list ap;
                va_start(ap, flags);
                mode = va_arg(ap, mode_t);
                va_end(ap);
    }
	
    // Intercept msr-safe opens
    if (isMSRSafeFile(pathname)) {    
        fprintf( stdout, "BLR: %s:%d Intercepted open() call to MSR Device %s\n",
		    	__FILE__, __LINE__, pathname );
	// TODO: Make the file descriptor CPU dependant
        return 2000;     
    }

    //Otherwise open normally
    else {
	    return real_open( pathname, flags, mode );
    }
}

int
close(int fd){
	return real_close(fd);
}


// 
// Intercept calls to fake file descriptor 
//
ssize_t
pread(int fd, void *buf, size_t count, off_t offset){

    if (fd == 2000){
        /*
         * Retrieve the struct msr  of requested MSR from stored MSR's in array MSRDAT
         * Then get the value of the msr address
         * if it is invalid return an error otherwise return size of address in bytes (8).
         */
        if (offset < 0) {
            errno = EDOM;
            return -1;
        }

        // Validate offset
        if ( (size_t) offset > sizeof(MSRDAT)/sizeof(MSRDAT[0])){
            errno = EDOM; // Not sure if this is the right error for this.
            return -1;
        }
        
        //Validate count
        if (count > 8) {
            errno = EOVERFLOW; // Not sure about this one either
            return -1;
        }

        if (MSRDAT[offset].valid) {
            *(uint64_t*) buf = MSRDAT[offset].value;
            return sizeof(uint64_t);
        }
        else {
            errno = EADDRNOTAVAIL;
            return -1;
        }
    }
    return real_pread( fd, buf, count, offset );
}


ssize_t
pwrite(int fd, const void *buf, size_t count, off_t offset){
    /*
     * v1 Write the value to the array if the MSR is valid
     * v2 Parse the msr allowlist to see if the MSR is writeable 
     * v3 Parse the msr allowlist to get the write mask and apply it
     *
     */

    if (fd == 2000) {

        if ( (size_t) offset > sizeof(MSRDAT)/sizeof(MSRDAT[0])){ 
            errno = EDOM; // Again sure if this is the best error for this.
            return -1;
        } 

        // Verify the MSR is allowed in the allowlist.
        if (MSRDAT[offset].valid && MSRDAT[offset].isAllowed) {
              // Apply the allowlist mask and write.
              // THIS LINE IS A BIT QUESTIONABLE.
              MSRDAT[offset].value = MSRDAT[offset].writeMask & *(uint64_t*) buf;
              return sizeof(uint64_t);
        }
        else {
            errno = EADDRNOTAVAIL;
            return -1;
        }
    }
     // If not calling our MSR's
     else {
	    return real_pwrite( fd, buf, count, offset );
    }
}

int
ioctl(int fd, unsigned long request, ...){
	// This is going to be a bit squirrely, as there's no way of knowing
	// how many arguments are being passed along unless we're able to
	// inspect the code at the receiving end.  Traditionally, the third
	// and final argument in a char *argp---the man page says so---
	// but if you're seeing weird ioctl bugs, your driver might be of a
	// less traditional bent.
	char *arg_p = NULL;
	va_list ap;
	va_start(ap, request);
	arg_p = va_arg(ap, char*);
	va_end(ap);

	if( fd==BATCH_FD && request==X86_IOC_MSR_BATCH ){
		// do batch processing here.
		return 0;
	}else{
		return real_ioctl( fd, request, arg_p );
	}
}

int
stat(const char *pathname, struct stat *statbuf){
	// The only think that msr_core.c:stat_module() checks is for
	// S_IRUSR and S_IWUSR flags in statbuf.st_mode. All we'll do
	// here is make sure it's an msr-related file, set those two
	// bits and return success.
	//
	// Note that it might nice to have a user interface that allows
	// for more flexibility during testing.

	int rc=0;
	int dummy_idx = 0;

	if(
		( 0 == strncmp( pathname, MSR_ALLOWLIST_PATH, strlen(MSR_ALLOWLIST_PATH) ) )
		||
		( 0 == strncmp( pathname, MSR_BATCH_PATH, strlen(MSR_BATCH_PATH) ) )
		||
		( 1 == sscanf( pathname, MSR_STOCK_PATH_FMT, &dummy_idx ) )
		||
		( 1 == sscanf( pathname, MSR_SAFE_PATH_FMT, &dummy_idx ) )
	){
		statbuf->st_mode |= ( S_IRUSR | S_IWUSR );
		return rc;
	}else{
		return real_stat( pathname, statbuf );
	}
}

