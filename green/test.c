#include <stdio.h>
#include <time.h>
#include "green.h"

int flag = 1;
int count = 0;
green_mutex_t mutex;
green_cond_t cond;

void *testBasic(void *arg) {
  int i = *(int*)arg;
  int loop = 100000;
  while(loop > 0 ) {
    loop--;
    green_yield();
  }
}

void *testInterrupts(void *arg) {
  int i = *(int*)arg;
  int loop = 100000;
  while(loop > 0 ) {
    loop--;
  }
}

void *testConditional(void *arg) {
  int id = *(int*)arg;
  int loop = 100000;
  while(loop > 0 ) {
    if(flag == id) {
      loop--;
      flag = (id + 1) % 2;
      green_cond_signal(&cond);
    } else {
      green_cond_wait_basic(&cond);
    }
  }
}

void *testConditionalLock(void *arg){
  int id = *(int*)arg;
  int loop = 100000;
  while(loop > 0) {
  green_mutex_lock(&mutex);
  count++; //new line
  while(flag != id) {
    green_cond_wait(&cond, &mutex);
  }
  flag = (id + 1) % 2;
  green_cond_signal(&cond);
  green_mutex_unlock(&mutex);
  loop--;
}
}

void *testLock(void *arg) {
  int i = *(int*)arg;
  int loop = 100000;
  while(loop > 0 ) {
    green_mutex_lock(&mutex);
    count++;
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
  double lengths[20];
  double tot = 0.0;
  // make the threads run 20 time to get some more interesting statistics
  for(int i = 0; i < 20; i++){
    clock_t begin = clock();
    green_create(&g0, testConditionalLock, &a0);
    green_create(&g1, testConditionalLock, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    clock_t end = clock();
    double time = (double)(end - begin) / CLOCKS_PER_SEC;
    count = 0;
    tot += time;
    lengths[i] = time;
  }
  printf("The total average %f", tot/20);
  FILE *file = fopen("timeConditionalLock.dat", "w");
  for(int i = 0; i < 20; i++) {
    fprintf(file, "%f\n", lengths[i]);
  }
  fclose(file);
  return 0;
}
