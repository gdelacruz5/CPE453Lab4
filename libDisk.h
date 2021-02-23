#ifndef LIBSISK_H
#define LIBDISK_H

#define BLOCKSIZE 256

int openDisk(char *filename, int nBytes);

int readBlock(int disk, int bNum, void *block);

int writeBlock(int disk, int bNum, void *block);

void closeDisk(int disk);

#endif
