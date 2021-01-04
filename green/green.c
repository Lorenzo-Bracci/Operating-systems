#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include "green.h"

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096

#include <signal.h>
#include <sys/time.h>

#define PERIOD 100

static sigset_t block;


static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE};

static green_t *running = &main_green;

//used to keep track of the bottom of the queue
struct green_t *bottom = NULL;

static void init() __attribute__((constructor));

void timer_handler(int sig);

void init() {
  getcontext(&main_cntx);
  sigemptyset(&block);
  sigaddset(&block, SIGVTALRM);

  struct sigaction act = {0};
  struct timeval interval;
  struct itimerval period;

  act.sa_handler = timer_handler;
  assert(sigaction(SIGVTALRM, &act, NULL) == 0);

  interval.tv_sec = 0;
  interval.tv_usec = PERIOD;
  period.it_interval = interval;
  period.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &period, NULL);
  getcontext(&main_cntx);
}

void timer_handler(int sig) {
  green_t * susp = running;
  // add susp to ready queue
  if(bottom == NULL){
    running->next = susp;
    bottom = susp;
  }else{
    bottom->next = susp;
    bottom = susp;
  }
  // select the next thread for execution
  green_t *next = susp->next;
  running = next;
  swapcontext(susp->context, next->context);
}

int green_mutex_init(green_mutex_t *mutex) {
  mutex->taken = FALSE;
  mutex->list = NULL;
  mutex->bottom = NULL;
}

int green_mutex_lock(green_mutex_t *mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t *susp = running;
  if(mutex->taken) {
    struct list *lst = (list *)malloc(sizeof(list));
    lst->current = running;
    lst->next = NULL;
   //insert the new element in the suspended queue
    if(mutex->list == NULL){
      mutex->list = lst;
      mutex->bottom = lst;
    }else{
      mutex->bottom->next = lst;
      mutex->bottom = lst;
    }
    // select the next thread for execution
    green_t *next = susp->next;
    running = next;
    swapcontext(susp->context, next->context);
  } else {
    // take the lock
    mutex->taken = 1;
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_mutex_unlock(green_mutex_t *mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  if(mutex->list != NULL) {
     // move suspended thread to ready queue
     green_t * susp = mutex->list->current;
     if(susp != NULL){
       // add susp to ready queue
       if(bottom == NULL){
         running->next = susp;
         bottom = susp;
       }else{
         bottom->next = susp;
         bottom = susp;
       }
       list * temp =  mutex->list;
       mutex->list = mutex->list->next;
       //free list node that we do not need anymore
       free(temp);
       // if list is empty update the pointer to the last element of the queue
       if(mutex->list == NULL){
         mutex->bottom = NULL;
       }
     }

  } else {
    mutex->taken = 0;
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

void green_thread() {
   green_t *this = running;
   void *result = (*this->fun)(this->arg);
   // place waiting (joining) thread in ready queue
   if(this->join != NULL){
     if(bottom == NULL){
       running->next = this->join;
       bottom = this->join;
     }else{
       bottom->next = this->join;
       bottom = this->join;
     }
   }
   // save result of execution
   this->retval = result;
   // we're a zombie
   this->zombie = 1;
   // find the next thread to run
   green_t *next = this->next;
   running = next;
   setcontext(next->context);
}

void green_cond_init(green_cond_t* cond){
  cond->list = NULL;
  cond->bottom = NULL;
}

void green_cond_wait_basic(green_cond_t* cond){
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t *this = running;
  struct list *lst = (list *)malloc(sizeof(list));
  lst->current = this;
  lst->next = NULL;
 //insert the new element in the suspended queue
  if(cond->list == NULL){
    cond->list = lst;
    cond->bottom = lst;
  }else{
    cond->bottom->next = lst;
    cond->bottom = lst;
  }
   sigprocmask(SIG_UNBLOCK, &block, NULL);
  // yield execution
    green_yield();
}

void green_cond_wait_without_lock(green_cond_t* cond){
//  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t *this = running;
  struct list *lst = (list *)malloc(sizeof(list));
  lst->current = this;
  lst->next = NULL;
 //insert the new element in the suspended queue
  if(cond->list == NULL){
    cond->list = lst;
    cond->bottom = lst;
  }else{
    cond->bottom->next = lst;
    cond->bottom = lst;
  }

}


int green_cond_wait(green_cond_t *cond, green_mutex_t *mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t * susp = running;
  //suspend the thread on the conditional variable
  green_cond_wait_without_lock(cond);
  if(mutex != NULL) {
    if(mutex->taken == 1){
      if(mutex->list != NULL) {
         // move suspended thread to ready queue
         green_t * susp = mutex->list->current;
         if(susp != NULL){
           // add susp to ready queue
           if(bottom == NULL){
             running->next = susp;
             bottom = susp;
           }else{
             bottom->next = susp;
             bottom = susp;
           }
           list * temp =  mutex->list;
           mutex->list = mutex->list->next;
           //free list node that we do not need anymore
           free(temp);
           // if list is empty update the pointer to the last element of the queue
           if(mutex->list == NULL){
             mutex->bottom = NULL;
           }
         }
      } else {
        mutex->taken = 0;
      }
    }

  }
  // schedule the next thread
  green_t * next = susp->next;
  running = next;
  swapcontext(susp->context, next->context);
  // get runninng thread
  susp = running;
  if(mutex != NULL) {
    // try to take the lock
      if(mutex->taken) {
        struct list *lst = (list *)malloc(sizeof(list));
        lst->current = running;
        lst->next = NULL;
       //insert the new element in the suspended queue
        if(mutex->list == NULL){
          mutex->list = lst;
          mutex->bottom = lst;
        }else{
          mutex->bottom->next = lst;
          mutex->bottom = lst;
        }
        green_t *next = susp->next;
        running = next;
        swapcontext(susp->context, next->context);


    } else {
      // take the lock
      mutex->taken = 1;
    }
  }
  // unblock
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

void green_cond_signal(green_cond_t* cond){
  sigprocmask(SIG_BLOCK, &block, NULL);
  if(cond->list != NULL){
  green_t * susp = cond->list->current;
  if(susp != NULL){
    // add susp to ready queue
    if(bottom == NULL){
      running->next = susp;
      bottom = susp;
    }else{
      bottom->next = susp;
      bottom = susp;
    }
    list * temp =  cond->list;
    cond->list = cond->list->next;
    //free list node that we do not need anymore
    free(temp);
    if(cond->list == NULL){
      cond->bottom = NULL;
    }
  }
}
   sigprocmask(SIG_UNBLOCK, &block, NULL);
}

int green_create(green_t *new, void *(*fun)(void*), void *arg) {
  ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
  getcontext(cntx);

  void *stack = malloc(STACK_SIZE);

  cntx->uc_stack.ss_sp = stack;
  cntx->uc_stack.ss_size = STACK_SIZE;
  makecontext(cntx, green_thread, 0);

  new->context = cntx;
  new->fun = fun;
  new->arg = arg;
  new->next = NULL;
  new->join = NULL;
  new->retval = NULL;
  new->zombie = FALSE;

  if(bottom == NULL){
    running->next = new;
    bottom = new;
  }else{
    bottom->next = new;
    bottom = new;
  }

  return 0;
}



int green_yield() {
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t * susp = running;
  // add susp to ready queue
  if(bottom == NULL){
    running->next = susp;
    bottom = susp;
  }else{
    bottom->next = susp;
    bottom = susp;
  }
  // select the next thread for execution
  green_t *next = susp->next;
  running = next;
  swapcontext(susp->context, next->context);
   sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_join(green_t *thread, void **res) {
  sigprocmask(SIG_BLOCK, &block, NULL);

  if(!thread->zombie) {
    green_t *susp = running;
    // add as joining thread
    thread->join = susp;
    //select the next thread for execution
    green_t *next = susp->next;
    running = next;
    swapcontext(susp->context, next->context);
  }
  // collect result
   res = thread->retval;
  // free context
    free(thread->context);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}
