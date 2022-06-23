
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#ifndef SYSCALL_WRAPPERS_H_
#define SYSCALL_WRAPPERS_H_


    void* w_malloc();
    void* w_calloc();
    void* w_realloc(void *ptr, size_t size);
    
    int w_open(const char* pathname, int flags);
    int w_open_p(const char* pathname, int flags, int permissions);
    int w_close(int fd);
    int w_read(int fd, void* buf, size_t count);
    int w_write(int fd, const void* buf, size_t count);
    
    int w_lseek(int fd, off_t offset, int whence);
    int w_unlink(const char* pathname);


    void w_lockf(int fd, int cmd, off_t len);
    void w_unlockf(int fd, int cmd, off_t len);


#endif
