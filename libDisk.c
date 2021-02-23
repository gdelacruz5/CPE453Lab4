#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "TinyFS_errno.h"
#include "libDisk.h"
#include "linkedList.h"

int diskNum = 1; //global for the disk numbers
ListNode *diskHead = NULL; //global linked list for the open disks

//checks to see if the file is already opened
int isOpen(char *filename){

    ListNode *ptr = diskHead;

    while (ptr != NULL)
    {
        if ((strcmp(filename, ptr->filename) == 0) && ptr->open)
        {
            return -1;
        }
        ptr = ptr->next;
    }

    return 0;
}

//opens an existing disk that is closed, returns the diskNum of the disk on success
int openExistingDisk(char *filename){

   ListNode *ptr = diskHead;

   while (ptr != NULL)
    {
        if (strcmp(filename, ptr->filename) == 0)
        {
            ptr->open = 1;
            if ((ptr->fp = fopen(ptr->filename, "r+")) == NULL)
            {
               //fprintf(stderr, "fopen error\n");
               return -1;
            }
            return ptr->diskNum;
        }
        ptr = ptr->next;
    }

    //fprintf(stderr, "disk does not exist\n");
    return NON_EXISTENT_DISK;
}

//opens a file as a disk, giving nBytes of space for writing/reading
int openDisk(char *filename, int nBytes){

    FILE *fp;
    char *name;

    //nBytes must be >= 0
    if (nBytes < 0)
    {
        return INVALID_DISK_SIZE;
    }
    
    //if nBytes is zero then open an existing disk
    if (nBytes == 0)
    {
       return openExistingDisk(filename);
    }

    if (isOpen(filename) != 0)
    {
        //for testing
        //fprintf(stderr, "file is already open\n");
        return DISK_ALREADY_OPEN;
    }

    if ((fp = fopen(filename, "w+")) == NULL)
    {
        //for testing
        //fprintf(stderr, "fopen error\n");
        return FOPEN_ERROR;
    }

    //will have to change this to account for disks that are in the thing but unmounted (ie they have been closed)
    //so the fopen are should be r+
    //will have to check the list for closed disks then set fp to fopen again

    name = malloc(strlen(filename) + 1);

    memset(name, '\0', (strlen(filename) + 1));
    strcpy(name, filename);

    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        //fprintf(stderr, "fseek error when moving pointer to block\n");
        return -4;
    }  

    diskHead = addTail(diskHead, diskNum, fp, nBytes, name);

    diskNum++;

    //for testing
    //printList(diskHead);
    
    return (diskNum - 1);
}

//reads a block (bNum) from a disk (disk) into a given pointer (*block)
//return 0 on success, -number on error
int readBlock(int disk, int bNum, void *block){

    FILE *fp;
    ListNode *ptr;

    //if the node is not in the list then error
    if ((ptr = getNode(diskHead, disk)) == NULL)
    {
        return NON_EXISTENT_DISK; //disk is not open
    }
    
    if (!ptr->open)
    {
       fprintf(stderr, "disk is closed\n");
       return CLOSED_DISK;
    }

    if ((bNum < 0) || (bNum >= ptr->size))
    {
        return BLOCK_OUT_OF_BOUNDS; //bNum out of bounds
    }

    fp = ptr->fp;

    if (fseek(fp, (bNum * BLOCKSIZE), SEEK_SET) != 0)
    {
        //fprintf(stderr, "fseek error when moving pointer to block\n");
        return FSEEK_ERROR;
    }

    if (fread(block, sizeof(char), 256, fp) != 256)
    {
        //fprintf(stderr, "fread error when reading block\n");
        return FREAD_ERROR;
    }

    return 0;
}

//writes BLOCKSIZE bytes into the disk(disk) at the block (bNum) from the pointer given (*block)
int writeBlock(int disk, int bNum, void *block){
 
    FILE *fp;
    ListNode *ptr;

    //if the node is not in the list then error
    if ((ptr = getNode(diskHead, disk)) == NULL)
    {
        return NON_EXISTENT_DISK; //disk is not open
    }

    if (!ptr->open)
    {
       //fprintf(stderr, "disk is closed\n");
       return CLOSED_DISK;
    }

    if ((bNum < 0) || (bNum >= ptr->size))
    {
        return BLOCK_OUT_OF_BOUNDS; //bNum out of bounds
    }

    fp = ptr->fp;

    if (fseek(fp, (bNum * BLOCKSIZE), SEEK_SET) != 0)
    {
        //fprintf(stderr, "fseek error when moving pointer to block\n");
        return FSEEK_ERROR;
    }

    if (fwrite(block, sizeof(char), 256, fp) != 256)
    {
        //may not need these error cases since the spec says behavior is undefined
        //fprintf(stderr, "fwrite error when reading block\n");
        return FWRITE_ERROR;
    }

    return 0;
}

//closes the disk specified by "disk"
void closeDisk(int disk){
   
    if (disk < 1)
    {
       //for testing
       //printf("invalid disk number\n");
       return;
    }
    ListNode *ptr = diskHead;

    ptr = getNode(diskHead, disk);

    //check to see if it closed properly, exit failure if not
    if (fclose(ptr->fp) != 0)
    {
        //fprintf(stderr, "fclose error\n");
        return;
        //should I change this to a return?
    }
      
    ptr->open = 0;
}
