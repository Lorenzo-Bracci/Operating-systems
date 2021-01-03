#include <stdio.h>
#include "green.h"

int flag = 1;
green_mutex_t mutex;
green_cond_t cond;

void *test(void *arg){
  int id = *(int*)arg;
  int loop = 10000000;
  while(loop > 0) {
//  printf("thread %d: %d\n", id, loop);
  green_mutex_lock(&mutex);
  while(flag != id) {
//    green_mutex_unlock(&mutex);
  //  printf("%s, %d\n", "waiting", id);
    green_cond_wait(&cond, &mutex);
  //  printf("%s, %d\n", "done waiting", id);
  //  green_mutex_lock(&mutex);
  }
  flag = (id + 1) % 2;
//  printf("%s by %i to %i \n", "the flag was updated", id, flag);
  green_cond_signal(&cond);
  green_mutex_unlock(&mutex);
  loop--;
}
}

int main() {
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;
  green_cond_init(&cond);
  green_mutex_init(&mutex);
  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);

  green_join(&g0, NULL);
  green_join(&g1, NULL);
  printf("Both threads finished\n");
  return 0;
}

 /*//PROGRAM SHOWING PROBLEMS THAT PRESENT THEMSELVES WHEN ACCESSING A SHARED DATA STRUCTURE
int count = 0;
green_mutex_t lock;

void *test(void *arg) {
  int i = *(int*)arg;
  int loop = 10000000;
  while(loop > 0 ) {
    count++;
    green_mutex_unlock(&lock);
    loop--;
  }
}

int main() {
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;
  green_mutex_init(&lock);
  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);

  green_join(&g0, NULL);
  green_join(&g1, NULL);
  printf("The final count is %i\n", count);
  return 0;
}*/



/* // TEST SHOWING THAT INTERRUPTS WORK
void *test(void *arg) {
  int i = *(int*)arg;
  int loop = 10;
  while(loop > 0 ) {
    printf("thread %d: %d\n", i, loop);
    for(int i = 0; i < 10000000; i++){
      i = i;
    }
    loop--;
  }
}

int main() {
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;
  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);

  green_join(&g0, NULL);
  green_join(&g1, NULL);
  printf("done\n");
  return 0;
}
*/
/* // TEST FOR CONDITIONAL VARIABLES
int flag = 0;
green_cond_t cond;

void *test(void *arg) {
  int id = *(int*)arg;
  int loop = 4;
  while(loop > 0 ) {
    if(flag == id) {
      printf("thread %d: %d\n", id, loop);
      loop--;
      flag = (id + 1) % 2;
      green_cond_signal(&cond);
    } else {
      green_cond_wait(&cond);
    }
  }
}
int main() {
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;
  green_cond_init(&cond);
  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);

  green_join(&g0, NULL);
  green_join(&g1, NULL);
  printf("done\n");
  return 0;
}*/
