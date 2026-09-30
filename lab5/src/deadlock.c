#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *Thread1(void *arg) {
  printf("Thread 1: lock mutex1\n");
  pthread_mutex_lock(&mutex1);

  sleep(1);

  printf("Thread 1: waiting for mutex2\n");
  pthread_mutex_lock(&mutex2);

  printf("Thread 1: got mutex2\n");

  pthread_mutex_unlock(&mutex2);
  pthread_mutex_unlock(&mutex1);

  return NULL;
}

void *Thread2(void *arg) {
  printf("Thread 2: lock mutex2\n");
  pthread_mutex_lock(&mutex2);

  sleep(1);

  printf("Thread 2: waiting for mutex1\n");
  pthread_mutex_lock(&mutex1);

  printf("Thread 2: got mutex1\n");

  pthread_mutex_unlock(&mutex1);
  pthread_mutex_unlock(&mutex2);

  return NULL;
}

int main(void) {
  pthread_t thread1;
  pthread_t thread2;

  pthread_create(&thread1, NULL, Thread1, NULL);
  pthread_create(&thread2, NULL, Thread2, NULL);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  return 0;
}