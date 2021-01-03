#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <math.h>
#define TRUE 1
#define FALSE 0
#define HEAD  (sizeof(struct head))
#define MIN(size) (((size)>(8))?(size):(8))
#define LIMIT(size) (MIN(0) + HEAD + size)
#define MAGIC(memory) ((struct head*)memory - 1)
#define HIDE(block)  (void*)((struct head* )block + 1)
#define ALIGN 8
#define ARENA (64*1024)

struct head {
  uint16_t bfree;  //  2 bytes, the status of block before
  uint16_t bsize;  //  2 bytes, the size of block before
  uint16_t free;   //  2 bytes, the status of the block
  uint16_t size;   //  2 bytes, the size (max 2^16 i.e. 64 Ki byte)
  struct head *next;   //  8 bytes pointer
  struct head *prev;   //  8 bytes pointer
};

struct head *after(struct head *block) {
  return (struct head*)((char*) block + block->size + HEAD);
}

struct head *before(struct head *block) {
  return (struct head*)((char*) block - block->bsize + HEAD);
}

struct head *split(struct head *block, int size) {
  int rsize = block->size - (HEAD + size); //remaining size
  block->size = size; //the old part of the block will have an updated size

  struct head *splt = after(block);
  splt->bsize = size;
  splt->bfree = FALSE;
  splt->size = rsize;
  splt->free = TRUE;

  struct head *aft = after(splt);
  aft->bsize = rsize;

  return splt;
}

struct head *arena = NULL;

struct head *new() {

  if(arena != NULL) {
    printf("one arena already allocated \n");
    return NULL;
  }

  // using mmap, but could have used sbrk
  struct head *new = mmap(NULL, ARENA,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(new == MAP_FAILED) {
    printf("mmap failed \n");
    return NULL;
  }

  /* make room for head and dummy */
  uint16_t size = ARENA - 2*HEAD;

  new->bfree = FALSE; //the block before the first block is not accessible
  new->bsize = 0; //the size of the block before the first block is zero (unaccessible block)
  new->free= TRUE; //this block is accessible
  new->size = size; //the size of the block is an arena minues 2 heads

  struct head *sentinel = after(new);

  sentinel->bfree = TRUE; //the block before is available
  sentinel->bsize = size; //the size of the block before is an arena minus two
  sentinel->free = FALSE; //this sentinal block is unavailable
  sentinel->size = 0; //it s empty

  /* this is the only arena we have */
  arena = (struct head*)new;

  return new;
}


struct head *flist;

int search(struct head *block) {
int done = FALSE;
struct head *current = flist;
  while(!done && current != NULL){
    if(current == block){
      done = TRUE;
    }else{
      current = current->next;
    }
  }

  return done;
}

void sanity() {
 struct head *current = flist;
 while(current != NULL){
   if(!current->free){
     printf("A non free block is in the free list\n");
   }
   if(current->size % ALIGN != 0){
      printf("The size of the block is not a multiple of align\n");
   }
   current = current->next;
 }
}

void arenaSanity() {
 struct head *current = arena;
 while(current->size != 0){
   struct head *next = after(current);
   if(current->size != next->bsize){
     printf("%i\n", current->size);
     printf("%i\n", next->bsize);
     printf("Size mistmatch\n");
   }
   if(current->free && !search(current)){
      printf("Free block not present in the linked list\n");
   }
   current = after(current);
 }
}

void detach(struct head *block) {
if(block == flist){
  flist = block->next;
}
  if (block->next != NULL){
    block->next->prev = block->prev;
  }
  if (block->prev != NULL){
    block->prev->next = block->next;
  }
  return ;
}

void insert(struct head *block) {
  struct head *current = flist;
  while(current != NULL){
    if(current == block){
      printf("%s\n", "err");
    }
    current = current->next;
  }
  block->next = flist;
  block->prev = NULL;
  if (flist != NULL){
    flist->prev = block;
  }
  flist = block;
}

int adjust(size_t request) {
  int size = 0;
  while(size < (int) request){ // make size an even multiple of align
    size += 2 * ALIGN;
  }
  return size;
}

struct head *find(int size) {
int notDone = TRUE;
struct head *current = flist;
  while(notDone && current != NULL){
    if(current->size >= size){ //check if we found an appropriate block
      notDone = FALSE; //used to finish the loop
      detach(current); //detach current block
      if(current->size >= 2 * size){ // check if it s twice as large
        struct head *newBlock = split(current, size); //spli block
        insert(newBlock);
      }
      current->free = FALSE; // mark current block as not free
      after(current)->bfree = FALSE; // keep reulsts consistent
    }else{
      current = current->next;
    }
  }

  return current;
}

struct head *merge(struct head *block) {

  struct head *aft = after(block);

  if (block->bfree) {
    struct head *bef = before(block);
    detach(bef);
    bef->size = block->size + block->bsize; // change the size
    aft->bsize = bef->size; //update next pointer
    block = bef; //update the block
  }

  if(aft->free) {
    detach(aft);
block->size = block->size + aft->size; //change size
after(block)->bsize = block->size; //update block after for coherency
  }
  return block;
}

int freeListSize(){
  int size = 0;
  struct head *current = flist;
    while(current != NULL){
        current = current->next;
        size++;
    }

    return size;
}

float avgSize(){
  int size = 0;
  int res = 0;
  struct head *current = flist;
    while(current != NULL){
if(current->size < 20000){
  res += current->size;
  size++;
}
        current = current->next;
    }
if(size == 0){
  return 0.0;
}else{
return (res / size);
}
}

void *dalloc(size_t request) {
  if(arena == NULL){ // JUST FOR TRIAL
  struct head *newArena = new();
  insert(newArena);
}
  if( request <= 0 ){
    return NULL;
  }
  int size = adjust(request);
  struct head *taken = find(size);
printf("The average block size in the free list is %f\n", avgSize());
  arenaSanity();
sanity();
  if (taken == NULL)
    return NULL;
  else
    return (char*) taken + HEAD;
}

void dfree(void *memory) {

  if(memory != NULL) {
    struct head *block = (struct head*)((char*) memory - HEAD);
    block = merge(block);
    struct head *aft = after(block);
    block->free = TRUE;
    aft->bfree = TRUE;
    insert(block);

  }
  return;
}
