#include "syscall_wrappers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

void* w_malloc(int size){
    void *data = NULL;
    if((data = malloc(size)) == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return data;
}


void* w_calloc(int size, int sizeof_elem){
    void *data = NULL;
    if((data = calloc(size, sizeof_elem)) == NULL){
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    return data;
}


void* w_realloc(void *ptr, size_t size){
    void *data = NULL;
    if((data = realloc(ptr, size)) == NULL){
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    return data;
}


int w_open(const char* pathname, int flags){
    int fd = open(pathname, flags);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fd;
}


int w_open_p(const char* pathname, int flags, int permissions){
    int fd = open(pathname, flags, permissions);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fd;
}


int w_close(int fd){
    int ret = close(fd);
    if(ret == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
    return ret;
}


int w_read(int fd, void* buf, size_t count){
    int ret = read(fd, buf, count);
    if(ret == -1){
        perror("read");
        exit(EXIT_FAILURE);
    }
    return ret;
}


int w_write(int fd, const void* buf, size_t count){
    int ret = write(fd, buf, count);
    if(ret == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }
    return ret;
}


int w_lseek(int fd, off_t offset, int whence){
    int ret = lseek(fd, offset, whence);
    if(ret == -1){
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    return ret;
}


int w_unlink(const char* pathname){
    int ret = unlink(pathname);
    if(ret == -1){
        perror("unlink");
        exit(EXIT_FAILURE);
    }
    return ret;
}


void w_lockfR(int fd, int cmd, off_t len){
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    fcntl(fd, F_SETLKW, &lock);
}


void w_unlockf(int fd, int cmd, off_t len){
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
}
