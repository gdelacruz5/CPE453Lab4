#ifndef TINYFS_ERRNO_H
#define TINYFS_ERRNO_H

//libDisk
#define INVALID_DISK_SIZE -1
#define DISK_ALREADY_OPEN -2
#define NON_EXISTENT_DISK -3
#define FOPEN_ERROR -4
#define FSEEK_ERROR -5
#define CLOSED_DISK -6
#define BLOCK_OUT_OF_BOUNDS -7
#define FREAD_ERROR -8
#define FWRITE_ERROR -9

//libTinyFS
#define INVALID_DISK_NAME -1
#define INVALID_FILE_NAME -1
#define OPEN_DISK_ERROR -2
#define DISK_ALREADY_MOUNTED -3
#define NO_DISK_BY_THAT_NAME -4
#define INCONSISTENT_DISK -5
#define NO_DISK_MOUNTED -6
#define READBLOCK_ERR -7
#define FILE_ALREADY_OPEN -8
#define INVALID_FD -9
#define INSUFFICIENT_BLOCKS -10
#define INVALID_FP -11
#define INVALID_OFFSET -12


#endif
