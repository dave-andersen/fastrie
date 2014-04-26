#include "tsqueue.hpp"
#include <iostream>

ts_queue<int, 2000> tq;

void *producer_thread(void *arg) {
  for (int i = 0; i < 3000; i++) {
    tq.push_back(1);
  }
  printf("Producer thread put 3000 on stack\n");
  return NULL;
}

void *consumer_thread(void *arg) {
  sleep(2);
  for (int i = 0; i < 1000; i++) {
    tq.pop_front();
  }
  printf("Consumer thread got 1000\n");
  return NULL;
}

int main() {
  pthread_t threads[4];
  pthread_create(&threads[0], NULL, producer_thread, NULL);
  sleep(1);
  printf("After 1 second of producer running, tq.size is: %d\n", tq.size());
  for (int i = 1; i < 4; i++) {
    pthread_create(&threads[i], NULL, consumer_thread, NULL);
  }
  for (int i = 0; i < 4; i++) {
    pthread_join(threads[i], NULL);
  }
}
