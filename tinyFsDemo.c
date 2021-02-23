#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tinyFS.h"

void printOptions(){

   printf("a) Make a new file system\n");
   printf("b) Mount a file\n");
   printf("c) Unmount\n");
   printf("d) Open a file\n");
   printf("e) Close a file\n");
   printf("f) Write to a file (limited to strings of 300 bytes)\n");
   printf("g) Delete a file\n");
   printf("h) Read a byte from a file\n");
   printf("i) Move a file pointer\n");
   printf("j) Rename a file\n");
   printf("k) List the files in the disk\n");
   printf("l) See a file's creation time\n");
   printf("q) Exit the program\n");
}

int demoLoop()
{
   char toWrite[1000];
   int toWriteSize;
   char choice;
   char name[8];
   int returnVal;
   int size;
   int fd;
   char read;
   int offset;

   printf("\nWhat would you like to do?: \n");

   printOptions();

   printf("\nYour choice: ");

   scanf(" %c", &choice);

   switch(choice) {
      case 'a' :
         printf("Enter disk name: ");
         scanf(" %s", name);
         printf("Enter the size in blocks: ");
         scanf(" %d", &size);
         if ((returnVal = tfs_mkfs(name, size)) < 0)
         {
            printf("mkfs error\n");
            break;
         }
         break;
      case 'b' :
         printf("Enter the disk name: ");
         scanf(" %s", name);
         if ((returnVal = tfs_mount(name)) < 0)
         {
            printf("mount error\n");
            break;
         }
         break;
      case 'c' :
         if (tfs_unmount())
         {
            printf("no disk mounted\n");
            break;
         }
         break;
      case 'd' :
         printf("Enter the file name: ");
         scanf(" %s", name);
         if ((returnVal = tfs_openFile(name)) < 0)
         {
            printf("open file error\n");
            break;
         }
         else
         {
            printf("The file descriptor is: %d\n", returnVal);
            break;
         }
      case 'e' :
         printf("Enter the file descriptor: ");
         scanf(" %d", &fd);
         if (tfs_closeFile(fd) < 0)
         {
            printf("close file error\n");
            break;
         }
         break;
      case 'f' :
         printf("Enter the file descriptor: ");
         scanf(" %d", &fd);
         printf("Enter the file contents (limited to 1000 bytes): ");
         scanf(" %[^\n]", toWrite);
         toWriteSize = strlen(toWrite);
         if (tfs_writeFile(fd, toWrite, toWriteSize) < 0)
         {
            printf("write file error\n");
            break;
         }
         break;
      case 'g' :
         printf("Enter the file descriptor: ");
         scanf(" %d", &fd);
         if (tfs_deleteFile(fd) < 0)
         {
            printf("delete file error\n");
            break;
         }
         break;
      case 'h' :
         printf("Enter the file descriptor: ");
         scanf(" %d", &fd);
         if (tfs_readByte(fd, &read))
         {
            printf("read byte error\n");
            break;
         }
         printf("The byte is: 0x%02x\n", read);
         break;
      case 'i' :
         printf("Enter the file descriptor: ");
         scanf(" %d", &fd);
         printf("Enter the offset: ");
         scanf(" %d", &offset);
         if (tfs_seek(fd, offset) < 0)
         {
            printf("seek error\n");
            break;
         }
         break;
      case 'j' :
         printf("Enter the file descriptor: ");
         scanf(" %d", &fd);
         printf("Enter the new name: ");
         scanf(" %s", name);
         if (tfs_rename(fd, name) < 0)
         {
            printf("rename error\n");
         }
         break;
      case 'k' :
         printf("The files are: \n");
         if ((returnVal = tfs_readdir()) < 0)
         {
            printf("no disk mounted\n");
            break;
         }
         break;
      case 'l' :
         printf("Enter the file descriptor: ");
         scanf(" %d", &fd);
         printf("The file's creation time is: %s\n", tfs_readFileInfo(fd));
         break;
      case 'q' :
         printf("Good Bye!\n\n");
         return 0;
      default :
         printf("Invalid input\n");
   }

   return 1;
}

int main(){

   while(demoLoop())
   {
   }
   //char diskName[] = "test3";
   //char buf[253];
   //int diskSize = 6;
   //int i;
   //int choice;

   //printf("Welcome to the tinyFS demo!\n");
   //print options
   //printf("What would you like to do?: ");
   //scanf(" %d", &choice);
   
   /*for(i = 0; i < 253; i++)
   {
      buf[i] = 0xFF;
   }
   
   tfs_mkfs(diskName, diskSize);
   printf("mount return value: %d\n", tfs_mount("test3"));
  
   printf("the return value of openFIle is: %d\n", tfs_openFile("thing"));

   scanf("%d", &diskSize);
   //printf("opening new file stuff, return val is: %d\n", tfs_openFile("stuff"));
   printf("we are writing file return value is: %d\n", tfs_writeFile(0, buf, sizeof(buf)));

   printf("time stuff: %s\n", tfs_readFileInfo(0));


   tfs_openFile("stuff");

   tfs_unmount();
   tfs_mount("test3");
   tfs_readdir();
   
   tfs_rename(1, "bloop");

   tfs_readdir();

   printf("unmount return value: %d\n", tfs_unmount());*/

   return 0;
}
