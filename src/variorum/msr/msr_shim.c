// Copyright 2020-2021 Lawrence Livermore National Security, LLC and other
// // Variorum Project Developers. See the top-level LICENSE file for details.
// //
// // SPDX-License-Identifier: MIT

/* msr_fake.c
 * Compile:
 * gcc -Wextra -Werror -shared -fPIC -ldl -o msr_shim.so msr_shim.c
 *
 * Use:
 * LD_PRELOAD=./msr_shim.so ~/variorum_shim/variorum/build_lupine/examples/variorum-print-frequency-example
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
#include "../variorum_topology.h"


const int DUMMYFD = 2000; // Used as the file descriptor for any MSR call.

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


/**
 * Evaulate a filepath to odetermine if it is 
 * a cpu msr file path. 
 * 
 * @param pathname a file path to evaluate 
 * @return True if file path is of MSR type else false
 *
 */
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


/**
 * Evaulate bash enviromental variable MSRFORCEDERROR 
 * and return relevant errno values
 * 
 * I am not 100% sure this works. This might not necessarily
 * be broken but it's use  in pread() probably is.
 *
 * @return A relevant integer error or 0 if MSRFORCEDERROR is unset
 *
 * TODO: Verify & / or  change errcode values and refactor to follow
 * format of batch_msr_Errors() if deemed appropriate. 
 */
int
msrErrors(){
    int errcode = 0;
    char* my_env_var = getenv("MSRFORCEDERROR");
    if (NULL == my_env_var) {
        return errcode;
    }
    if (strcmp(my_env_var, "MSRDOESNOTEXIST") == 0 ) {
        errcode = 5;
    }
    else if (strcmp(my_env_var,"MSRNOTINALLOWLIST") == 0 ){
        errcode = 13;
    }
    else if (strcmp(my_env_var, "MSRNOTPERMITTED") == 0 ){
        errcode = 13;
    }
    else if (strcmp(my_env_var, "MSRSAFENONEXISTANT") == 0) {
        errcode = 2;
    }
    return errcode;
}


/**
 * Evaulate bash enviromental variable MSRFORCEDERROR 
 * and return relevant msr batch errno values
 *  
 * I might be confusing error types here. Use of method in IOCTL is
 * nonfunctional as the differences between the error types and what / how 
 * to set array op errors is unclear. 
 *
 * @return A relevant integer error or 0 if MSRFORCEDERROR is unset.
 *
 * TODO: Verify & / or change errcode values and fix use of this in IOCTL so it will actually
 * Interrupt and set error codes correctly.
 */
static long  
batch_msr_Errors(){
    /*
     * batch_msr_errors evaluates envirometal variable MSRFORCEDERROR 
     * for the following possible errors that can be forced by the shim user
     *
     */
    char* my_env_var = getenv("MSRFORCEDERROR");
    if (NULL == my_env_var) {
       return 0;
    }
    if (strcmp(my_env_var, "BADIOCTL") == 0 ) {
        return -ENOTTY;
    }
    if (strcmp(my_env_var, "FILENOTOPEN") == 0 ) {
        return -EBADF;
    }
    if (strcmp(my_env_var, "BADNUMOPS") == 0 ) {
        return -EINVAL;
    }
    if (strcmp(my_env_var, "NOMEMORY") == 0 ) {
        return -ENOMEM;
    }
    if (strcmp(my_env_var, "CPBATCHDESCRIPTORFAIL") == 0 ||
        strcmp(my_env_var, "CPBATCHARRAYFAIL") == 0) {
        return -EFAULT;
    }
    return 0;
}


//
// Interceptors
//


/**
 * Intercept open calls made 
 *
 * Intecept open calls and identify if the open call is to an /dev/cpu/#/msr 
 * file. In this instance pass back a dummy file descriptor otherwise normally
 * open the file  
 *
 * @propety pathname, The file path of files being attempted to open.
 * @return DUMMYFD if file path is identified as MSR 
 * otherwise the true file descriptor.
 *
 * TODO: 
 * Make the file descriptor CPU dependant and generate dummy
 * File desciptors based on CPU Number.
 */
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
        return DUMMYFD; 
    }

    //Otherwise open normally
    else {
	    return real_open( pathname, flags, mode );
    }
}


/**
 * Intercept close calls  
 *
 * Intecept close calls and if they are to the dummy file descriptor return sucess 
 * (because the dummy file descriptor was never a real file opened in the first place.
 *
 * @propety fd, File descriptor to close
 * @return 0 if success  
 *
 */
int
close(int fd){
    if (fd = DUMMYFD){
        return 0;
    }
	return real_close(fd);
}



/**
 * Intercept calls to pread on the fake file descriptor and return emulated data.  
 *
 * Retrieve the struct msr of the requested MSR from stored MSR's in array MSRDAT
 * Then get the value of the msr address and if it is invalid return an error 
 * otherwise return size of address in bytes (8).
 
 * @propety fd, File descriptor DUMMYFD (2000) if shim
 * @property buf, what to write the read results into
 * @property count, size of MSR data
 * @property offset, the offset of the MSR
 * @return 8 if success  
 *
 * TODO: use of msrErrors() does not seem to currently interrupt this method or
 * operate as intended
 */
ssize_t
pread(int fd, void *buf, size_t count, off_t offset){

    if (fd == DUMMYFD){
        //TODO:make msrErrors interrupt pread if an error occurs and correctly set error value.
        if (msrErrors() != 0) {
            errno = msrErrors();
            return -1;
        }

        if (offset < 0) { // Validate offset
            errno = EDOM;
            return -1;
        }

        // Validate offset
        if ( (size_t) offset > sizeof(MSRDAT)/sizeof(MSRDAT[0])){
            errno = EDOM; //TODO: Not sure if this is the right error for this.
            return -1;
        }
        
        // Validate count
        if (count > 8) {
            errno = EOVERFLOW; //TODO: Not sure about this one either
            return -1;
        }
        // Read Values
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



/**
 * Intercept pwrite calls on the fake file descriptor and write them to emulated data.  
 *
 * Intercepts pwrite calls, checks the allowlists to see if the MSR is in the writemask
 * and then write to the emulated MSR following Allowlist and writemask rules.
 *
 * @propety fd, File descriptor DUMMYFD if shim
 * @property buf, what to write the read results into
 * @property count, size of MSR data
 * @property offset, the offset of the MSR
 * @return 8 if success  
 *
 * TODO: use of msrErrors() needs to be extended to this method and all it's
 * possible resultant errors and then a way of calling it needs to be added.
 */
ssize_t
pwrite(int fd, const void *buf, size_t count, off_t offset){
    /*
     * v1 Write the value to the array if the MSR is valid
     * v2 Parse the msr allowlist to see if the MSR is writeable 
     * v3 Parse the msr allowlist to get the write mask and apply it
     *
     */

    if (fd == DUMMYFD) {

        if ( (size_t) offset > sizeof(MSRDAT)/sizeof(MSRDAT[0])){ 
            errno = EDOM; // Again sure if this is the best error for this.
            return -1;
        } 

        // Verify the MSR is allowed in the allowlist.
        if (MSRDAT[offset].valid && MSRDAT[offset].isAllowed) {
              // Apply the allowlist mask and write.
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



/**
 * Intercept ioctl calls on perform ioctl operations on emulated data.  
 *
 * Intercepts ioctl calls, and run through IOCTL batch operations on emulated data
 * following the writemask rules in the op struct.
 *
 * @propety fd, File descriptor DUMMYFD if shim
 * @property buf, what to write the read results into
 * @property arg_p, msr_batch_array
 * @return batch op results  
 *
 * TODO: Use of forced_errs does not currently work as this does not operate on errno.
 * Find what err constant to set and set that to result of forced_errs.
 * Extent emulation data to handle multiple CPUs.
 */

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
    struct msr_batch_array *p = (struct msr_batch_array *) &arg_p;
    struct msr_batch_op *op;
    int res = 0;
    uint64_t writeResultMSR;
    long int forced_errs = batch_msr_Errors();


	if( fd==BATCH_FD && request==X86_IOC_MSR_BATCH ){
        for (op = p->ops; op < p->ops + p->numops; ++op) { //Iterate setting op
                
            if (forced_errs != 0) {
                errno = forced_errs;
                continue;
           }        
                       
           if (op->cpu > variorum_get_num_cores()){ // If CPU is invalid
                errno = ENXIO;
                res = op->err; //Maybe not correct
                continue;
           }
           
           if (!MSRDAT[op->msr].valid || !MSRDAT[op->msr].isAllowed){
                errno = EFAULT;
                //op->err = //TODO: Not Sure what to set this as.
                continue;
           }
           if (op->isrdmsr) { // Read MSR
               op->msrdata = MSRDAT[op->msr].value; 
               res = op->err;
               continue;
           }
           else { //Write MSR
               writeResultMSR = op->wmask & op->msrdata;
               MSRDAT[op->msr].value = MSRDAT[op->msr].writeMask & writeResultMSR; 
               res = op->err;
               continue;
            }
        }
		return res;
	}
    else{
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

