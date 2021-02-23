#include <stdio.h>
#include <stdlib.h>
#include "linkedList.h"

/* Need to add function to find and return *fp or ListNode* for a given page
 * Function for deleting an entire list
 * Function to set a *fp?
 */

ListNode* addHead(ListNode *list, int diskNum, FILE *fp, int size, char *filename)
{
   ListNode *head = malloc(sizeof(ListNode));
   
   if (head == NULL)
   {
      fprintf(stderr, "malloc failure attempting to add %d\n", diskNum);
      exit(EXIT_FAILURE);
   }

   head->diskNum = diskNum;
   head->fp = fp;
   head->size = size;
   head->filename = filename;
   head->open = 1;
   head->next = list;
   return head;
}

ListNode* getTail(ListNode *list)
{
   ListNode *ptr = list;
   
   if (ptr == NULL)
   {
      return NULL;
   }
      
   while (ptr->next !=NULL)
   {
      ptr = ptr->next;
   }

   return ptr;
}

ListNode* addTail(ListNode *list, int diskNum, FILE *fp, int size, char *filename)
{
   ListNode *tail = malloc(sizeof(ListNode));
   ListNode *ptr;

   if (tail == NULL)
   {
      fprintf(stderr, "malloc failure attempting to add %d\n", diskNum);
      exit(EXIT_FAILURE);
   }
   
   ptr = getTail(list);
   if (ptr == NULL)
   {
      list = tail;
   }
   else
   {
      ptr->next = tail;
   }

   tail->diskNum = diskNum;
   tail->fp = fp;
   tail->size = size;
   tail->filename = filename;
   tail->open = 1;
   tail->next = NULL;

   return list;
}

int getMaxIndex(ListNode *list)
{
   int maxIndex = 0;
   ListNode *ptr = list;

   while (ptr != NULL)
   {
      ptr = ptr->next;
      maxIndex++;
   }

   return maxIndex;
}

ListNode* deleteNode(ListNode *list, int index)
{
   int maxIndex = getMaxIndex(list);
   int i;
   ListNode *prev = list;
   ListNode *delete;

   if ((index < 0) || (index >= maxIndex))
   {
      printf("Index %d is out of bounds\n", index);
      return list;
   }

   if (maxIndex == 1)
   {
      free(list);
      return NULL;
   }

   if (index == 0)
   {
      delete = list;
      list = list->next;
      delete->next = NULL;
      free(delete);
      return list;
   }

   for (i = 0; i < index - 1; i++)
   {
      prev = prev->next;
   }

   delete = prev->next;

   prev->next = delete->next;

   delete->next = NULL;

   free(delete);

   return list;
}

ListNode* getNode(ListNode *list, int diskNum)
{
    ListNode *ptr = list;

    while (ptr != NULL)
    {
        if (ptr->diskNum == diskNum)
        {
            return ptr;
        }
        ptr = ptr->next;
    }

    return NULL;
}

int getIndex(ListNode *list, int diskNum)
{
   int index = 0;
   ListNode *ptr = list;

   while (ptr != NULL)
   {
      if (ptr->diskNum == diskNum)
      {
         return index;
      }
      index++;
      ptr = ptr->next;
   }

   return -1;
}

void printList(ListNode *list)
{
   ListNode *ptr = list;
   int space = 0;
   printf("List: ");
   if(ptr == NULL)
   {
      printf("Empty\n");
      return;
   }
   while (ptr != NULL)
   {
      printf(space ? " " : "");
      printf("%d", ptr->diskNum);
      ptr = ptr->next;
      space = 1;
   }
   printf("\n");
}
