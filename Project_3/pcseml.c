#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "eventbuf.h"

int producer_count;
int consumer_count;
int producer_events_count;
int outstanding_count;

sem_t *mutex;
sem_t *items;
sem_t *spaces;

struct eventbuf *eb;  

sem_t *sem_open_temp(const char *name, int value)
{
    sem_t *sem;

    // Create the semaphore
    if ((sem = sem_open(name, O_CREAT, 0600, value)) == SEM_FAILED)
        return SEM_FAILED;

    // Unlink it so it will go away after this process exits
    if (sem_unlink(name) == -1) {
        sem_close(sem);
        return SEM_FAILED;
    }

    return sem;
}

void *producer_run(void *arg) {
  int *producer_number = arg;

  for (int i = 0; i < producer_events_count; i++) { // Wait to see if there's enough space in the event buffer to post.
    int event = *producer_number * 100 + i; 
    sem_wait(spaces);
    sem_wait(mutex); // Lock a mutex around the eventbuf.
    printf("P%d: adding event %d\n",*producer_number, event); // Print that it's adding the event, along with the event number. This should match the sample output, above.
    eventbuf_add(eb, event); // Add an event to the eventbuf.
    sem_post(mutex); // Unlock the mutex.
    sem_post(items); // Signal waiting consumer threads that there is an event to be consumed.
  }
  printf("P%d: exiting", *producer_number);
  return NULL;
}

void *consumer_run(void *arg) {
  int *consumer_number = arg;

  while(1) { // Wait to see if a producer has put anything in the buffer.
    sem_wait(items);
    sem_wait(mutex); // Lock a mutex around the eventbuf.
    if(eventbuf_empty(eb)) { // Check to see if the eventbuf is empty.
      sem_post(mutex); // If it is, we're done and it's time to exit our loop. Be sure to unlock the mutex first.
      printf("C%d: exiting\n", *consumer_number);
      break;
    }
    int event = eventbuf_get(eb);// Since the eventbuf wasn't empty, get an event from it.
    printf("C%d: got event %d\n", *consumer_number, event); // Print a message about the event being received. This should match the sample output, above.
    sem_post(mutex); // Unlock the mutex
    sem_post(spaces); // If we're not done, post to the semaphore indicating that there are now free spaces for producers to put events into.
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  // Parse command line
  if (argc != 5) {
      fprintf(stderr, "usage: producer_count consumer_count producer_events_count outstanding_count\n");
      exit(1);
  }

  producer_count = atoi(argv[1]);
  consumer_count = atoi(argv[2]);
  producer_events_count = atoi(argv[3]);
  outstanding_count = atoi(argv[4]);

  eb = eventbuf_create(); // Create the event buffer
  mutex = sem_open_temp("mutex_sem", 1);
  items = sem_open_temp("items_sem", 0);
  spaces = sem_open_temp("spaces_sem", outstanding_count);

  pthread_t *producer_thread = calloc(producer_count, sizeof *producer_thread);
  int *producer_thread_id = calloc(producer_count, sizeof *producer_thread_id);
  pthread_t *consumer_thread = calloc(consumer_count, sizeof *consumer_thread);
  int *consumer_thread_id = calloc(consumer_count, sizeof *consumer_thread_id);

  for (int i = 0; i < producer_count; i++){   // Start the correct number of producer threads
    producer_thread_id[i] = i;
    pthread_create(producer_thread + i, NULL, producer_run, producer_thread_id + i); // Each thread will be passed a pointer to an int, its ID number
  }
  


  for (int i = 0; i < consumer_count; i++){   // Start the correct number of consumer threads
    consumer_thread_id[i] = i;
    pthread_create(consumer_thread + i, NULL, consumer_run, consumer_thread_id + i); // Each thread will be passed a pointer to an int, its ID number
  }


  for (int i = 0; i < producer_count; i++) {   // Wait for all producer threads to complete
    pthread_join(producer_thread[i], NULL);
  }

  for (int i = 0; i < consumer_count; i++) {   // Notify all the consumer threads that they're done
    sem_post(items);
  }

  for (int i = 0; i < consumer_count; i++) {  // Wait for all consumer threads to complete
    pthread_join(consumer_thread[i], NULL);
  }

  eventbuf_free(eb); // Free the event buffer
  return 1;
}
