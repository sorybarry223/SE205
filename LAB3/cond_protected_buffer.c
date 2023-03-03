#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "circular_buffer.h"
#include "protected_buffer.h"
#include "utils.h"

// Initialise the protected buffer structure above. 
protected_buffer_t * cond_protected_buffer_init(int length) {
  protected_buffer_t * b;
  b = (protected_buffer_t *)malloc(sizeof(protected_buffer_t));
  b->buffer = circular_buffer_init(length);
  // Initialize the synchronization components
  pthread_mutex_init(&b->m,NULL); //Initialize the mutex in protected_buffer_t structure
  pthread_cond_init(&b->empty,NULL);  //Initialize the cond var in protected_buffer_t structure
  pthread_cond_init(&b->full,NULL);
  return b;
}

// Extract an element from buffer. If the attempted operation is
// not possible immedidately, the method call blocks until it is.
void * cond_protected_buffer_get(protected_buffer_t * b){
  void * d;
  
  // Enter mutual exclusion
  pthread_mutex_lock(&b->m);
  
  // Wait until there is a full slot to get data from the unprotected
  // circular buffer (circular_buffer_get).
   
    while((d = circular_buffer_get(b->buffer))==NULL)
      pthread_cond_wait(&b->full,&b->m);
    
    

  // Signal or broadcast that an empty slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->empty);

  
  print_task_activity ("get", d);
  // Leave mutual exclusion
  pthread_mutex_unlock(&b->m);
  return d;
}

// Insert an element into buffer. If the attempted operation is
// not possible immedidately, the method call blocks until it is.
void cond_protected_buffer_put(protected_buffer_t * b, void * d){

  // Enter mutual exclusion
  pthread_mutex_lock(&b->m);
  // Wait until there is an empty slot to put data in the unprotected
  // circular buffer (circular_buffer_put).
  
  while(circular_buffer_put(b->buffer,d)==0)
    pthread_cond_wait(&b->empty,&b->m);
  
  // Signal or broadcast that a full slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->full);

  circular_buffer_put(b->buffer, d);
  print_task_activity ("put", d);

  // Leave mutual exclusion
  pthread_mutex_unlock(&b->m);
}

// Extract an element from buffer. If the attempted operation is not
// possible immedidately, return NULL. Otherwise, return the element.
void * cond_protected_buffer_remove(protected_buffer_t * b){
  void * d;
  

  // Signal or broadcast that an empty slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->empty);

  d = circular_buffer_get(b->buffer);
  print_task_activity ("remove", d);
  
  return d;
}

// Insert an element into buffer. If the attempted operation is
// not possible immedidately, return 0. Otherwise, return 1.
int cond_protected_buffer_add(protected_buffer_t * b, void * d){
  int done;
  
  // Enter mutual exclusion
  pthread_mutex_lock(&b->m);
  // Signal or broadcast that a full slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->full);

  done = circular_buffer_put(b->buffer, d);
  if (!done) d = NULL;
  print_task_activity ("add", d);

  // Leave mutual exclusion
  pthread_mutex_unlock(&(b->m));

  return done;
}

// Extract an element from buffer. If the attempted operation is not
// possible immedidately, the method call blocks until it is, but
// waits no longer than the given timeout. Return the element if
// successful. Otherwise, return NULL.
void * cond_protected_buffer_poll(protected_buffer_t * b, struct timespec *abstime){
  void * d = NULL;
  int    rc=0 ;
  
  // Enter mutual exclusion
  pthread_mutex_lock(&b->m);
  // Wait until there is an empty slot to put data in the unprotected
  // circular buffer (circular_buffer_put) but waits no longer than
  // the given timeout.
  
  while(b->buffer->size==0 && rc !=ETIMEDOUT) {
    rc= pthread_cond_timedwait(&b->empty,&b->m, abstime);
  }
  // Signal or broadcast that a full slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->full);

  d = circular_buffer_get(b->buffer);
  print_task_activity ("poll", d);

  // Leave mutual exclusion
  pthread_mutex_unlock(&(b->m));
  return d;
}

// Insert an element into buffer. If the attempted operation is not
// possible immedidately, the method call blocks until it is, but
// waits no longer than the given timeout. Return 0 if not
// successful. Otherwise, return 1.
int cond_protected_buffer_offer(protected_buffer_t * b, void * d, struct timespec * abstime){
  int rc = 0;
  int done = 0;
  // Enter mutual exclusion
  pthread_mutex_lock(&b->m);
  // Signal or broadcast that a full slot is available in the
  // unprotected circular buffer (if needed) but waits no longer than
  // the given timeout.
  //((((Le commentaire ci-dessus est erroné))))
    
    while(b->buffer->size==b->buffer->max_size && rc !=ETIMEDOUT) {
      rc= pthread_cond_timedwait(&b->full,&b->m, abstime);
  }

  // Signal or broadcast that a full(empty) slot is available in the
  // unprotected circular buffer (if needed)
//  pthread_cond_broadcast(&b->full);
  pthread_cond_broadcast(&b->empty);

  done = circular_buffer_put(b->buffer, d);
  if (!done) d = NULL;
  print_task_activity ("offer", d);
    
  // Leave mutual exclusion
  pthread_mutex_unlock(&(b->m));

  return done;
}


