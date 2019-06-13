#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <list>
#include <map>
#include <cstdio>
#include <bitset>
#include "disk.h"

#define MAX_INFO_BLOCK 4096
#define MAX_DATA_BLOCK 4096
#define MAX_ENTRY 4096
#define MAX_FILE 64

int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int umount_fs(char *disk_name);
int fs_open(char *name);
int fs_close(int fildes);
int fs_create(char *name);
int fs_delete(char *name);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);