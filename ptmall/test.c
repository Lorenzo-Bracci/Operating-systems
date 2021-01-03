#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <time.h>
#include "dlmall2.h"
#define MIN 8
#define MAX 200
#define REQUESTS 98


struct list {
  char *data;
  struct list *next;
};

struct list *listOfBlocks;

char *getRandom(int size){
  int i = 0;
  int random = rand() % size; //get a random int that represent the block that we will free
  struct list *current = listOfBlocks;
  if(random == 0){ // if random is zero then we simply work on the first pointer
char *ans = listOfBlocks->data;
    listOfBlocks = listOfBlocks->next;
    return ans;
  }
  while(i < random - 1){ //stop one position before the position that we want to remove
current = current->next;
    i++;
  }
  struct list *res = current->next; //output
  current->next = current->next->next; //remove the output from the list
  return res->data;
}

int main( ) {
int lengths[REQUESTS];
  int i;
  int blocksInUse = 0;
listOfBlocks = malloc(sizeof *listOfBlocks);
clock_t begin = clock();
for (i=0; i < REQUESTS; i++){ //60 times allocate some data and which a certain probabbility the same data will be deallocated
  int size = (rand() % (MAX - MIN + 1)) + MIN; //size of current block to be allocated
blocksInUse++; //increase the counter of blocks in use
char *data2 = dalloc(sizeof (char) * size); //allocated memory;
   struct list *current;
   current = malloc(sizeof *current);
current->data = data2;
current->next = listOfBlocks;
listOfBlocks = current;
lengths[i] = freeListSize();
if((rand() % 2) == 0){ //with given chance (50%) free memory that has been allocated
char *blockToFree = getRandom(blocksInUse);
blocksInUse--; //decrease the amount of blocks in use
  dfree(blockToFree);
  struct list *trial = listOfBlocks;
}
}
clock_t end = clock();
double time = (double)(end - begin) / CLOCKS_PER_SEC;
printf("The total time is %f", time);

FILE *file = fopen("lengths.dat", "w");


  for(int i = 0; i < REQUESTS; i++) {
    fprintf(file, "%d\n", lengths[i]);
  }

  fclose(file);

return 0;
 }
