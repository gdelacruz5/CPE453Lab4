#ifndef LINKEDLIST_H
#define LINKEDLIST_H

typedef struct node
{
   int diskNum; //the disk number
   FILE *fp; //FILE pointer to file
   int size; //the size in BLOCKS
   char *filename; //the name of the file
   int open;
   struct node *next;
} ListNode;

ListNode* addHead(ListNode *list, int diskNum, FILE *fp, int size, char *filename);

ListNode* addTail(ListNode *list, int diskNum, FILE *fp, int size, char *filename);

int getMaxIndex(ListNode *list);

ListNode* deleteNode(ListNode *list, int index);

ListNode* getNode(ListNode *list, int diskNum);

int getIndex(ListNode *list, int diskNum);

void printList(ListNode *list);

#endif
