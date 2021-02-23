#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "TinyFS_errno.h"
#include "tinyFS.h"
#include "libDisk.h"
#include "linkedList.h"

tfsFile fileTable[FILE_TABLE_SIZE];

int currentDisk = -1;

//writes the current time to the inode block specified. used for additional
//feature E
void writeTime(char inodeBlkNum, int offset)
{
   time_t rawtime;
   struct tm *info;
   char *timeString;
   int timeStrLen;
   char inode[BLOCKSIZE];
   int i;

   //write time last modified to inode block
   time( &rawtime );
   info = localtime( &rawtime );
   timeString = asctime(info);
   timeStrLen = (int)strlen(timeString);
   //printf("strlen time string = %d\n", timeStrLen);
   //printf("time: %s\n", timeString);

   readBlock(currentDisk, inodeBlkNum, inode);
   for(i = 0; i < timeStrLen; i++)
   {
      inode[i + offset] = timeString[i];
   }
   writeBlock(currentDisk, inodeBlkNum, inode);

}

///////////////////////////////////////////
///////   tfs_mkfs section ////////////////
///////////////////////////////////////////

//sets the free blocks on disk initialization
void setFreeBlocks(int diskNum, int nBytes)
{
   char freeBlock[BLOCKSIZE];
   int i;
   
   freeBlock[BLOCK_TYPE] = FREE_BLOCK; //block type
   freeBlock[DISK_FORMAT] = MAGIC_NUM;

   for(i = 2; i < BLOCKSIZE; i++)
   {
      freeBlock[i] = 0x00;
   }
   
   for(i = 1; i < nBytes - 1; i++)
   {
      freeBlock[2] = i + 1; //pointer to next free block
      writeBlock(diskNum, i, freeBlock);
   }
   
   freeBlock[2] = 0;
   writeBlock(diskNum, i, freeBlock); //last block has NULL pointer
}


//sets the superblock on disk initialization
void setSuperBlock(int diskNum, int nBytes)
{
   char superBlock[BLOCKSIZE];
   int i;

   superBlock[0] = 0x01; //block type
   superBlock[1] = 0x45; //magic num
   superBlock[2] = 0x01; //pointer to free block list
   superBlock[3] = 0x00; //pointer to inode list, starts at null 00
   superBlock[4] = nBytes; // number of blocks of the file
   
   //byte 5... init with 0s
   for(i = 5; i < BLOCKSIZE; i++)
   {
      superBlock[i] = 0;
   }

   writeBlock(diskNum, 0, superBlock);
}

//checks if the filename is composed only of alphanumeric characters and is les than 9 characters long
int checkFileName(char *filename)
{
    int length = 1;

    while(*filename)
    {
        if (((isdigit(*filename) == 0) && (isalpha(*filename) == 0)) || (length > 8) || (length == 0))
        {
            return -1;
        }

        filename++;
        length++;
    }

    return 0;
}

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes)
{
   char diskName[9];
   int diskNum; 
   
   //check the filename
   if (checkFileName(filename))
   {
      //fprintf(stderr, "filename must be only alphanumeric characters with a maximum length of 8 characters\n");
      return INVALID_FILE_NAME;
   }

   //should we do a check here to make suer nBytes > 0? or > 1?

   //open the disk
   if ((diskNum = openDisk(filename, nBytes)) < 0)
   {
      //fprintf(stderr, "could not open disk\n");
      return OPEN_DISK_ERROR;
   }

   strcpy(diskName, filename);
   
   setSuperBlock(diskNum, nBytes);

   setFreeBlocks(diskNum, nBytes);
  
   closeDisk(diskNum);

   return diskNum;
}

////////////////////////////////////////////////////
//////////// tfs_mount and tfs_unmount section /////
//////////// Also contains Additional feature H ////
////////////////////////////////////////////////////

//checks the block of the given type
int checkBlock(char block[], char type, char prev, char dataInodeP)
{
   if ((block[BLOCK_TYPE] != type) || (block[DISK_FORMAT] != MAGIC_NUM))
   {
      return -1;
   }

   if ((block[BLOCK_TYPE] == INODE_BLOCK) && (block[INODE_PREV] != prev))
   {
      return -1;
   }

   if ((block[BLOCK_TYPE] == FILE_EXTENT) && (block[INODE_NEXT] != dataInodeP))
   {
      return -1;
   }

   return 0;
}

//checks the data extents of an inode
int checkDataExtents(int diskNum, char dataExtentP, char inode)
{
   char dataExtentBlock[BLOCKSIZE];
   
   while (dataExtentP != 0x00)
   {
      readBlock(diskNum, dataExtentP, dataExtentBlock);
      if (checkBlock(dataExtentBlock, FILE_EXTENT, 0, inode) < 0)
      {
         return -1;
      }
      dataExtentP = dataExtentBlock[DATA_POINTER];
   }

   return 0;
}

//checks the inode blocks of the disk for consistency
int checkInodeBlocks(int diskNum, char inodeList)
{
   char inodeBlock[BLOCKSIZE];
   char prevInode = 0;

   while (inodeList != 0x00)
   {
      //for testing
      //printf("check inode: %d, with prev: %d\n", inodeList, prevInode);
      readBlock(diskNum, inodeList, inodeBlock);
      if (checkBlock(inodeBlock, INODE_BLOCK, prevInode, 0) < 0)
      {
         return -1;
      }
      if (checkDataExtents(diskNum, inodeBlock[DATA_POINTER], inodeList) < 0)
      {
         return -1;
      }
      prevInode = inodeList;
      inodeList = inodeBlock[INODE_NEXT];
   }

   return 0;
}

//checks the free blocks of the disk for consistency
int checkFreeBlocks(int diskNum, char freeBList)
{
   char freeBlock[BLOCKSIZE];

   while (freeBList != 0x00)
   {
      readBlock(diskNum, freeBList, freeBlock);
      if (checkBlock(freeBlock, FREE_BLOCK, 0, 0) < 0)
      {
         return -1;
      }
      freeBList = freeBlock[DATA_POINTER];
   }

   return 0;
}


//checks the disk requested with mount for consistency
int checkDisk(int diskNum)
{
   char superBlock[BLOCKSIZE];
   char freeBPointer;
   char inodeP;

   readBlock(diskNum, 0, superBlock); //read the superblock

   freeBPointer = superBlock[DATA_POINTER];

   if (checkFreeBlocks(diskNum, freeBPointer) < 0)
   {
      //free blocks not consistent
      //printf("free blocks is not consistent\n");
      return -1;
   }

   inodeP = superBlock[INODE_NEXT];

   if (checkInodeBlocks(diskNum, inodeP) < 0)
   {
      //inodes not consistent
      //printf("inodes not consistent\n");
      return -1;
   }

   return 0;
}

//returns the number of blocks of the disk, stored in the superblo k
int getNumBlocks(int diskNum)
{
   char block[BLOCKSIZE];

   if (readBlock(diskNum, 0, block) != 0)
   {
      //fprintf(stderr, "readblock error\n");
      return -1;
   }

   if ((block[0] != 1) && (block[1] != 0X45))
   {
      //fprintf(stderr, "superblock not correctly formatted\n");
      return -1;
   }
   else
   {
      return block[4]; //byte 4 of the superblock is the total number of blocks
   }
}

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’. tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount operation, tfs_mount should verify the file system is the correct type. Only one file system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted file system. Must return a specified success/error code. */
int tfs_mount(char *filename)
{
   int diskNum;
   int numBlocks; //used for consistency check
   int i;

   if (currentDisk != -1)
   {
      //fprintf(stderr, "there is already a disk mounted\n");
      return DISK_ALREADY_MOUNTED;
   }

   if ((diskNum = openDisk(filename, 0)) < 1)
   {
      //fprintf(stderr, "no disk by that name\n");
      return NO_DISK_BY_THAT_NAME;
   } 

   //gets the number of blocks and checks the super block is correct
   if ((numBlocks = getNumBlocks(diskNum)) < 1)
   {
      return INCONSISTENT_DISK;
   }

   if (checkDisk(diskNum) < 0)
   {
      //disk is not in correct format
      return INCONSISTENT_DISK;
   }

   //set all the table entries to open
   for (i = 0; i < 20; i++)
   {
      (fileTable[i]).open = 1;
   }

   //for testing
   //printf("the number of blocks of this disk is: %d\n", numBlocks);
   currentDisk = diskNum;

   //for testing
   //printf("current disk number: %d\n", currentDisk);
   return 0;
}

int tfs_unmount(void)
{ 
   int i;
   
   if (currentDisk == -1)
   {
      return NO_DISK_MOUNTED;
   }

   for(i = 0; i < 20; i++)
   {
      if (!fileTable[i].open)
      {
         //for testing
         //printf("\n\nclosing fd: %d\n\n", i);
         tfs_closeFile(i);
      }
   }
   closeDisk(currentDisk);

   currentDisk = -1;

   //for testing
   //printf("current disk number is: %d\n", currentDisk);
   return 0;
}

///////////////////////////////////////////////////////////
////////////// tfs_openFile section ///////////////////////
///////////////////////////////////////////////////////////

//sets the filename in the inode of the new file
void setFileName(char *name, char block[])
{
   int i;

   for(i = 0; name[i] != '\0'; i++)
   {
      block[5 + i] = name[i];
   }

   block[5 + i] = '\0';
}

//sets the file blocks on init
int setFileBlocks(int inodeP, int dataP, int lastInodeP)
{
   char block[256];
   int freeP;

   //set inode block
   if (readBlock(currentDisk, inodeP, block))
   {
      printf("setFileBlocks; readblock error\n");
      return -1;
   }

   //for testing
   //printf("\n this new inode is: %d", inodeP);
   //printf("\n the previous inode is: %d\n\n", lastInodeP);

   block[0] = 2;//set the block type
   block[3] = 0;//set pointer to next inode to NULL
   block[4] = lastInodeP; //set pointer to the previous inode in the list

   //sets size to zero, it's an int so it's 4 bloks
   block[16] = 0;
   block[17] = 0;
   block[18] = 0;
   block[19] = 0;

   if (writeBlock(currentDisk, inodeP, block))
   {
      //printf("setfileBlocks: writeblock error\n");
      return -1;
   }

   //set first data extent
   if (readBlock(currentDisk, dataP, block))
   {
      //printf("setFileBlocsk: readBlock error\n");
      return -1;
   }

   freeP = block[2]; //new beginning of free data block list
   
   block[0] = 3; //set the block type
   block[2] = 0; //set the pointer to the next extent block, init to 0
   block[3] = inodeP; //set the pointer to the inode blok

   if (writeBlock(currentDisk, dataP, block))
   {
      //printf("setfileBlocks: writeblock error\n");
      return -1;
   }

   return freeP;
}

//chreates a new inode and 1 empty data extent, returns the block number of the inode
int getNewInode(int lastInodeP)
{
   char block[256]; //block for reading and writing
   int freeP; //pointer to free blocks
   int inodeP; //pointer to new inode
   int dataP; //pointer to data extent
   int freeBlocks = 0; //counter for the number of free blocks available

   //read supre block
   if (readBlock(currentDisk, 0, block))
   {
      //for testing
      ////////////////printf("getnewinode: readblock error\n");
      return -1;
   }

   freeP = block[2];
   inodeP = freeP;

   //check freeP for two free bloks
   while (freeP != 0)
   {
      freeBlocks++; //increment the number of free blocks
      if (freeBlocks == 2)
      {
         break;
      }
      if (readBlock(currentDisk, freeP, block))
      {
         //for testing
         //printf("getNewInode: readblock error\n");
         return -1;
      }
      freeP = block[2];
      dataP = freeP;
   }

   if (freeBlocks == 2)
   {
      freeP = setFileBlocks(inodeP, dataP, lastInodeP);
   }
   else
   {
      //for teting
      //printf("getNewInode: there are only %d free blocks\n", freeBlocks);
      return -1;
   }

   if (readBlock(currentDisk, 0, block))
   {
      //for testing
      //printf("getNewInode: readBlock error\n");
      return -1;
   }

   block[2] = freeP;

   if (writeBlock(currentDisk, 0, block))
   {
      //printf("getNewInode: writeblock error\n");
      return -1;
   }

   //for testing
   //printf("inodeP is: %d, dataP is: %d\n", inodeP, dataP);
   //printf("free block list starts at: %d\n", freeP);
   return inodeP;
}

//checks if there is an open table entry, returns the index if so, -1 if there is none
fileDescriptor frstOpenEntry()
{
   int i;

   for (i = 0; i < 20; i++)
   {
      if ((fileTable[i]).open)
      {
         return i;
      }
   }

   return -1;
}

//adds a new inode to the inode list and inits the file with 1 file extent
//block that has no data
fileDescriptor addNewInode(int inodeP, int thisBlock, char *name)
{
   char block[256];
   int newInodeP;
   int fd; //the new fd


   //for testing
   //printf("\n\naddNewinode thisblock: %d\n\b", thisBlock);
   //check if there is space in the fd table
   if ((fd = frstOpenEntry()) < 0)
   {
      //for testing
      //printf("no available resources for file entry descriptor\n");
      return -1;
   }

   //get the new inode block pointer
   //newInodeP = getNewInode(inodeP);
   if ((newInodeP = getNewInode(thisBlock)) == -1)
   {
      //for testing
      //printf("not enough space for new file (needs 2 blocks)\n");
      return -1;
   }

   //printf("the newInode is: %d\n", newInodeP);

   //read the last inode block into "block"
   if (readBlock(currentDisk, thisBlock, block) < 0)
   {
      //printf("addnewInode: readblock error\n");
      return -1;
   }

   //for testing
   //printf("the last inode is: %d\n", thisBlock);
   //printf("the newInode is: %d\n", newInodeP);

   block[3] = newInodeP; //set the pointer to the next inode to the newly created inode

   //write the block back
   if (writeBlock(currentDisk, thisBlock, block) < 0 )
   {
      //printf("addNewinode: writeBlock error\n");
      return -1;
   }

   //set the name in the block
   //read the new inode block
   if (readBlock(currentDisk, newInodeP, block))
   {
      //printf("addNewInode: readBlock error\n");
      return -1;
   }

   setFileName(name, block);

   if (writeBlock(currentDisk, newInodeP, block))
   {
      //printf("addNewInode: writeBlock error\n");
      return -1;
   }

   //set creation and modification time
   writeTime(newInodeP, TIME_CRE_OFFSET);
   writeTime(newInodeP, TIME_MOD_OFFSET);

   //set file table entry
   (fileTable[fd]).inodeBlock = newInodeP; //set the inodePointer
   (fileTable[fd]).size = 0; //set the size, init to zero
   (fileTable[fd]).fp = 0; //set the fp, init to 0
   (fileTable[fd]).open = 0; //set this table entry to not open

   //for testing
   //printf("the new fd is: %d\n", fd);
   //printf("\n\nthis inode bNum: %d\n\n", newInodeP);

   return fd;
}

//opens a new file in the disk, returns an error if there is no space
fileDescriptor openNewFile(char *name, int inodeP, int thisBlock)
{
   char block[BLOCKSIZE];
   int newInodeP;

   if (inodeP > 0)
   {
      //for testing
      //printf("this inode is: %d\n", thisBlock);
      //printf("next inode is at block %d\n", inodeP);

      //otherwise read the inode block and get the next inodeP
      if (readBlock(currentDisk, inodeP, block) < 0)
      {
         //for testing
         //printf("readBlock error\n");
         return -1;
      }

      newInodeP = block[3];

      return openNewFile(name, newInodeP, inodeP);
   }
   else //if this is the ned of the inode list then add a new inode
   {
      //for testing
      //printf("openNewFile: this is the end of the inode list, adding new Inode\n");
      //printf("\ninodeP: %d, thisblock: %d\n\n", inodeP, thisBlock);
      return addNewInode(inodeP, thisBlock,  name);
   }
}

//sets the file table entry, returns the filedescriptor, -1 if no entries
fileDescriptor findOpenTableEntry(int inodeP)
{
   int i;

   for(i = 0; i < 20; i++)
   {
      if (!(fileTable[i]).open && ((fileTable[i]).inodeBlock == inodeP))
      {
         //for testing
         //printf("file is already open\n");
         return -2;
      }
   }

   for(i = 0; i < 20; i++)
   {
      if ((fileTable[i]).open) //else use the first open 
      {
         return i;
      }
   }
   return -1; //no entries available
}

//checks to see if the file is in the disk by the name, set a table entry if so
fileDescriptor checkName(char *name, char block[], int inodeP)
{
   char thisFileName[9];
   int i = 0;
   int j = 5;

   while((thisFileName[i] = block[j]) != 0x00)
   {
      i++;
      j++;
   }

   if (strcmp(name, thisFileName) == 0)
   {
      //for testing
      printf("checkanme: file in disk: %s\n", name);
      //also need to check if file is already open
      return findOpenTableEntry(inodeP);
   }
   return -1;
}

//checks the inode list of the disk for an existing file with "name"
fileDescriptor checkInodesForFile(char *name, int inodeP)
{
   //needs to be completed

   char block[256];
   int intBlock[64];
   int fd;
   int nextInodeP;

   //for teting
   //printf("check inodes for file: inodeP is %d\n", inodeP);

   //return -1 for not found in disk
   if (inodeP == 0)
   {
      //for testing
      //printf("checkInodesForFile: inodeP is 0, no files in disk\n");
      return -1;
   }

   //otherwise there is an inode in the disk so read it into "block"
   if (readBlock(currentDisk, inodeP, block) < 0)
   {
      //fprintf(stderr, "readBlock error\n");
      return -1;
   }

   //the file is in the disk, at block inodeP
   if ((fd = checkName(name, block, inodeP)) >= 0)
   {
      readBlock(currentDisk, inodeP, intBlock);
      //for testing
      //printf("the size of %s is: %d\n", name, intBlock[4]);
      //set table entry and return the file descriptor
      (fileTable[fd]).inodeBlock = inodeP;
      (fileTable[fd]).size = intBlock[4]; //how to pull the size from the block
      (fileTable[fd]).fp = 0;
      (fileTable[fd]).open = 0;
      return fd;
   }
   else if (fd == -2)
   {
      return -2;
   }

   nextInodeP = block[3]; //get the pointer to the next inode
   return checkInodesForFile(name, nextInodeP);
}

/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name)
{
   int fd;
   char block[256];
   int inodeP;

   //maybe check table entry here first to see if there are any

   if (currentDisk == -1)
   {
      //printf("no disk mounted\n");
      return NO_DISK_MOUNTED;
   }

   //check the name
   if (checkFileName(name))
   {
      return INVALID_FILE_NAME;
   }

   //read the superblock into "block"
   if (readBlock(currentDisk, 0, block) < 0)
   {
      //printf("readBlock error\n");
      return READBLOCK_ERR;
   }

   inodeP = block[3];

   //for testing
   //printf("the pointer to the first inode is: %d\n", inodeP);

   if ((fd = checkInodesForFile(name, inodeP)) < 0) //if it is not an existing file
   {
      if (fd == -2)
      {
         return FILE_ALREADY_OPEN;
      }
      //for testing
      //printf("file is not in disk, opening new file\n");
      //printf("openFile: inodeP: %d\n", inodeP);
      fd = openNewFile(name, inodeP, 0); //open a new file in the disk
   }

    return fd;
}

///////////////////////////////////////////////////////////////
//\//////////// tfs_ closeFile section ////////////////////////
///////////////////////////////////////////////////////////////
 
/*error check helper function to see if the filedescriptor is valid */
int isValidFD(fileDescriptor FD)
{
   if((FD < 0) || (FD >= 20) )
   {
      //error
      return -1;
   }else if (fileTable[FD].open == 1)
   { 
      //error
      return -1;
   }
   return 0;
}
 
/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD)
{
   if(isValidFD(FD) == -1)
   {
      //error
      return INVALID_FD;
   } 
   fileTable[FD].open = 1;
   writeTime(fileTable[FD].inodeBlock, TIME_ACC_OFFSET);
   return 0;
}

///////////////////////////////////////////////////////////
///////////////// tfs_writeFile section ///////////////////
///////////////////////////////////////////////////////////
 
/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. Sets the file pointer to 0 (the start of file) when done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size)
{
   char inode[BLOCKSIZE];
   int oldSize;
   int numBlocks;
   int i; 
   int j;
   char data[BLOCKSIZE];
   char freeBlock[BLOCKSIZE];
   char superBlock[BLOCKSIZE];
   char freeHead, lastBlock, newDataHead, curBlock;
   int sizeInBlocks;
   int oldSizeInBlocks;
   int intInode[BLOCKSIZE/4];

   
   if(isValidFD(FD) == -1)
   {
      //error
      return INVALID_FD;
   }
   
   oldSize = fileTable[FD].size;
   fileTable[FD].size = size;
   fileTable[FD].fp = 0;

   //block access
   readBlock(currentDisk, fileTable[FD].inodeBlock, inode);
   readBlock(currentDisk, fileTable[FD].inodeBlock, data);
   readBlock(currentDisk, 0, freeBlock);
   readBlock(currentDisk, 0, superBlock);

   //free block management
   sizeInBlocks = size/(BLOCKSIZE-4);
   oldSizeInBlocks = oldSize/(BLOCKSIZE-4);

   //needs more space 
   if(sizeInBlocks> oldSizeInBlocks)
   {
      //get more blocks

      //compute how many blocks needed
      numBlocks = sizeInBlocks -oldSizeInBlocks;
      //check if enough free blocks available
      for(i = 0; i < numBlocks; i++){
         if(freeBlock[2] == 0x00)
         {
            //fprintf(stderr, "not enough free blocks\n");
            return INSUFFICIENT_BLOCKS;
         }
         readBlock(currentDisk, freeBlock[2], freeBlock);
      }

      freeHead = superBlock[2];
   
      //traverse to end of existing data blocks   
      while(data[2] != 0)
      {
         lastBlock = data[2];
         readBlock(currentDisk, data[2], data);
      }
      //circumvent taken blocks for superblock
      superBlock[2] = freeBlock[2];
      writeBlock(currentDisk, 0, superBlock);
      data[2] = freeHead;
      writeBlock(currentDisk, lastBlock, data);
   }
   
   //needs less space
   else if(sizeInBlocks < oldSizeInBlocks)
   {
      //free some blocks
      numBlocks = oldSizeInBlocks - sizeInBlocks;
      freeHead = inode[2]; //removing blocks from head of data extent list
      for(i = 0; i < numBlocks; i++)
      {
         lastBlock = data[2];
         readBlock(currentDisk, data[2], data);
         data[0] = 0x04; //set to free block
         //set data to 0s
         for(j = 4; j < BLOCKSIZE; j++)
         {
            data[j] = 0x00;
         }
      }
      newDataHead = data[2];
      inode[2] = newDataHead;
      writeBlock(currentDisk, fileTable[FD].inodeBlock, inode);
      
      //adding the newly freed blocks to the HEAD of the free block list
      data[2] = superBlock[2];
      writeBlock(currentDisk, lastBlock, data);
      superBlock[2] = freeHead;
      writeBlock(currentDisk, 0, superBlock); 
   }

   //write data
   i = 0;
   readBlock(currentDisk, fileTable[FD].inodeBlock, data);
   while(i < size)
   {
      curBlock = data[2];
      readBlock(currentDisk, data[2], data);
      data[0] = 0x03;
      data[3] = fileTable[FD].inodeBlock;
      for(j = 4; (j < (BLOCKSIZE)) && (i < size); j++)
      {
         data[j] = buffer[i];
         i++;
      }
      writeBlock(currentDisk, curBlock, data);
   }
   //change last data block pointer to 0
   data[2] = 0x00;
   writeBlock(currentDisk, curBlock, data);

   //write size to inode
   readBlock(currentDisk, fileTable[FD].inodeBlock, intInode);
   intInode[4] = size;
   writeBlock(currentDisk, fileTable[FD].inodeBlock, intInode);
   writeTime(fileTable[FD].inodeBlock, TIME_MOD_OFFSET);
   return 0;
}

////////////////////////////////////////////////////////////////////////
//////////////////////// tfs_deleteFile section/////////////////////////
////////////////////////////////////////////////////////////////////////
 
/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD)
{
   char block[BLOCKSIZE];
   char freeBlock[BLOCKSIZE];
   int i, j;
   char here, next, prev;
   char lastFreeBlock;

   if(isValidFD(FD) == -1)
   {
      //error
      //printf("deleteFile: invalid fd");
      return INVALID_FD;
   }
   
   here = fileTable[FD].inodeBlock; //the inode fo the data to be deleted
   fileTable[FD].open = 1;
   readBlock(currentDisk, here, block);
   //block = inode
   next = block[3]; //pointer to next inode
   prev = block[4]; //pointer to prev inode

   //for testing
   //printf("\n\ndeleting next: %d, prev: %d\n\n", next, prev);
     
   //access superblock to acesss free block list
   readBlock(currentDisk, 0, freeBlock);
   lastFreeBlock = 0x00; //start at super block
   //traverse free block list to end
   while(freeBlock[2] != 0x00)
   {
      lastFreeBlock = freeBlock[2];
      readBlock(currentDisk, freeBlock[2], freeBlock);
   }
   freeBlock[2] = here;
   writeBlock(currentDisk, lastFreeBlock, freeBlock);

   //iterate through each data block
   //fileSize/BLOCKSIZE = num of blocks to clear
   for(i = 0; i < (fileTable[FD].size/(BLOCKSIZE - 4)) + 2; i++)
   {
      //printf("\n i: %d\n", i);
      //free data
      block[0] = 0x04; //block type
      for(j = 3; j < BLOCKSIZE; j++)
      {
         block[j] = 0x00;
      }
      writeBlock(currentDisk, here, block);
      
      //add current block to end of free block chain
      freeBlock[2] = here; //
      here = block[2];
      readBlock(currentDisk, block[2], block);
      readBlock(currentDisk, freeBlock[2], freeBlock);
   }
   freeBlock[2] = 0x00;
   //repair inode chain
   readBlock(currentDisk, prev, block);
   block[3] = next;
   writeBlock(currentDisk, prev, block);

   if (next != 0)
   {
      readBlock(currentDisk, next, block);
      block[4] = prev;
      writeBlock(currentDisk, next, block);
   }
   return 0;
}

/////////////////////////////////////////////////////////////
//////////// tfs_readByte and tfs_seek section///////////////
/////////////////////////////////////////////////////////////

/* reads one byte from the file and copies it to buffer, using the current file pointer location and incrementing it by one upon success. If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer)
{
   char block[BLOCKSIZE];
   int i;
   const int dataStart = 4; //data starts at byte 4
   if(isValidFD(FD) == -1)
   {
      //error
      return INVALID_FD;
   }
   readBlock(currentDisk, fileTable[FD].inodeBlock, block);
   
   //check if fp is at end
   if(fileTable[FD].fp >= fileTable[FD].size)
   {
      //error
      return INVALID_FP;
   }
   //traverse data blocks to reach offset location
   for(i = 0; i < fileTable[FD].fp / (BLOCKSIZE - dataStart) + 1; i++)
   {
      readBlock(currentDisk, block[2], block);
   }
   
   //retrieve byte
   *buffer = block[(fileTable[FD].fp % (BLOCKSIZE - dataStart)) + dataStart];
      
   //increment file pointer
   fileTable[FD].fp++;   
   return 0;
}
 
/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset)
{
   //char block[BLOCKSIZE];
   if(isValidFD(FD) == -1)
   {
      //error
      return INVALID_FD;
   }
   //readBlock(currentDisk, fileTable[FD].inodeBlock, block);
   if((offset > fileTable[FD].size) || (offset < 0))
   {
      return INVALID_OFFSET; //error
   }
   fileTable[FD].fp= offset;
   return 0;
}

//////////////////////////////////////////////////////////////
///////////// Additional Feature B///////////////////////////
/////////////////////////////////////////////////////////////

//b. Directory listing and renaming
int tfs_rename(fileDescriptor FD, char *newName) /* renames a file.  New name should be passed in. */
{
   char inode[BLOCKSIZE];

   if (currentDisk == -1)
   {
      return NO_DISK_MOUNTED;
   }

   if (isValidFD(FD) == -1)
   {
      return INVALID_FD;
   }
   
   if (checkFileName(newName) == -1)
   {
      return INVALID_FILE_NAME;
   }

   readBlock(currentDisk, fileTable[FD].inodeBlock, inode);
  
   setFileName(newName, inode); 

   writeBlock(currentDisk, fileTable[FD].inodeBlock, inode);
      
   return 0;
}

// prints the name of the file given the inode block data
void printFileName(char inode[])
{
   int i;
   
   for (i = 5; (i < 14) && (inode[i] != 0x00); i++)
   {
      printf("%c", inode[i]);
   }
   printf("\n");
   //printf("printed %d chars\n", i - 5);
}

int tfs_readdir() /* lists all the files and directories on the disk */
{
   char superBlock[BLOCKSIZE];
   char inode[BLOCKSIZE];
   char nextInode;

   if (currentDisk == -1)
   {
      return NO_DISK_MOUNTED;
   }

   readBlock(currentDisk, 0, superBlock);
   nextInode = superBlock[INODE_NEXT];

   while (nextInode != 0x00)
   {
      readBlock(currentDisk, nextInode, inode);
      printFileName(inode);
      nextInode = inode[INODE_NEXT];
   }
   
   return 0;
}

/////////////////////////////////////////////////
/////////// Additional Feature E/////////////////
/////////////////////////////////////////////////

/* returns the file’s creation time or all info (up to you if you want to make multiple functions) */
char * tfs_readFileInfo(fileDescriptor FD)
{
   char inode[BLOCKSIZE];
   char *time = malloc(25);
   int i;

   if (currentDisk == -1)
   {
      return "error, no disk mounted\n";
   }

   if (isValidFD(FD) == -1)
   {
      return "error, invalid fd\n";
   }

   readBlock(currentDisk, fileTable[FD].inodeBlock, inode);
   for (i = 0; i < 25; i++)
   {
      time[i] = inode[TIME_CRE_OFFSET + i];
   }
   
   return time;
}

