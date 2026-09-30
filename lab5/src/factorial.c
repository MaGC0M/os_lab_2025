#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct FactorialArgs {
  unsigned long long begin;
  unsigned long long end;
  unsigned long long mod;
};

unsigned long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *CalculateFactorial(void *arg) {
  struct FactorialArgs *args = (struct FactorialArgs *)arg;

  unsigned long long local_result = 1;

  for (unsigned long long i = args->begin; i <= args->end; i++) {
    local_result = (local_result * i) % args->mod;
  }

  pthread_mutex_lock(&mutex);

  result = (result * local_result) % args->mod;

  pthread_mutex_unlock(&mutex);

  return NULL;
}

int main(int argc, char **argv) {
  long long k = -1;
  int pnum = -1;
  long long mod = -1;

  static struct option options[] = {
      {"pnum", required_argument, 0, 'p'},
      {"mod", required_argument, 0, 'm'},
      {0, 0, 0, 0}
  };

  int c;

  while ((c = getopt_long(argc, argv, "k:", options, NULL)) != -1) {
    switch (c) {
      case 'k':
        k = atoll(optarg);
        break;

      case 'p':
        pnum = atoi(optarg);
        break;

      case 'm':
        mod = atoll(optarg);
        break;

      default:
        return 1;
    }
  }

  if (k < 0 || pnum <= 0 || mod <= 0) {
    printf(
        "Usage: %s -k num --pnum=num --mod=num\n",
        argv[0]
    );
    return 1;
  }

  if (mod == 1) {
    printf("Result: 0\n");
    return 0;
  }

  if (k == 0 || k == 1) {
    printf("Result: %lld\n", 1 % mod);
    return 0;
  }

  pthread_t *threads = malloc(sizeof(pthread_t) * pnum);
  struct FactorialArgs *args =
      malloc(sizeof(struct FactorialArgs) * pnum);

  if (threads == NULL || args == NULL) {
    perror("malloc");
    free(threads);
    free(args);
    return 1;
  }

  for (int i = 0; i < pnum; i++) {
    args[i].begin = (k * i) / pnum + 1;
    args[i].end = (k * (i + 1)) / pnum;
    args[i].mod = mod;

    if (pthread_create(
            &threads[i],
            NULL,
            CalculateFactorial,
            &args[i]
        ) != 0) {
      printf("pthread_create failed\n");
      free(threads);
      free(args);
      return 1;
    }
  }

  for (int i = 0; i < pnum; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("Result: %llu\n", result);

  pthread_mutex_destroy(&mutex);

  free(threads);
  free(args);

  return 0;
}