#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

static volatile sig_atomic_t timeout_happened = 0;

void AlarmHandler(int signal_number) {
  (void)signal_number;
  timeout_happened = 1;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1;
  bool with_files = false;

  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be positive\n");
              return 1;
            }
            break;

          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be positive\n");
              return 1;
            }
            break;

          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be positive\n");
              return 1;
            }
            break;

          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout must be positive\n");
              return 1;
            }
            break;
        }
        break;

      case 'f':
        with_files = true;
        break;

      default:
        return 1;
    }
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf(
        "Usage: %s --seed num --array_size num --pnum num "
        "[--timeout num] [-f]\n",
        argv[0]
    );
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  pid_t child_pids[pnum];
  bool child_finished[pnum];

  for (int i = 0; i < pnum; i++) {
    child_pids[i] = -1;
    child_finished[i] = false;
  }

  int active_child_processes = 0;
  int pipes[pnum][2];

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();

    if (child_pid < 0) {
      perror("fork");
      return 1;
    }

    if (child_pid == 0) {
      unsigned int begin = (array_size / pnum) * i;

      unsigned int end =
          (i == pnum - 1)
              ? (unsigned int)array_size
              : (array_size / pnum) * (i + 1);

      struct MinMax min_max = GetMinMax(array, begin, end);

      if (with_files) {
        char filename[64];
        sprintf(filename, "minmax_%d.txt", i);

        FILE *f = fopen(filename, "w");

        if (!f) {
          perror("fopen");
          exit(1);
        }

        fprintf(f, "%d %d\n", min_max.min, min_max.max);
        fclose(f);
      } else {
        close(pipes[i][0]);
        write(pipes[i][1], &min_max, sizeof(min_max));
        close(pipes[i][1]);
      }

      return 0;
    }

    child_pids[i] = child_pid;
    active_child_processes++;
  }

  if (timeout == -1) {
    /* Без timeout поведение остаётся старым. */
    while (active_child_processes > 0) {
      int status;

      if (wait(&status) == -1) break;

      active_child_processes--;
    }
  } else {
    signal(SIGALRM, AlarmHandler);
    alarm((unsigned int)timeout);

    while (active_child_processes > 0) {
      if (timeout_happened) {
        printf("Timeout reached. Killing child processes.\n");

        for (int i = 0; i < pnum; i++) {
          if (!child_finished[i]) {
            kill(child_pids[i], SIGKILL);
          }
        }

        /* Забираем убитых детей, чтобы не оставлять zombie. */
        for (int i = 0; i < pnum; i++) {
          if (!child_finished[i]) {
            waitpid(child_pids[i], NULL, 0);
            child_finished[i] = true;
          }
        }

        active_child_processes = 0;
        break;
      }

      int status;

      /* Не блокируем родителя, пока дети ещё работают. */
      pid_t finished_pid = waitpid(-1, &status, WNOHANG);

      if (finished_pid > 0) {
        for (int i = 0; i < pnum; i++) {
          if (child_pids[i] == finished_pid) {
            child_finished[i] = true;
            break;
          }
        }

        active_child_processes--;
      } else if (finished_pid == 0) {
        usleep(10000);
      } else {
        break;
      }
    }

    alarm(0);
  }

  if (timeout_happened) {
    if (!with_files) {
      for (int i = 0; i < pnum; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
      }
    } else {
      for (int i = 0; i < pnum; i++) {
        char filename[64];
        sprintf(filename, "minmax_%d.txt", i);
        unlink(filename);
      }
    }

    free(array);
    return 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[64];
      sprintf(filename, "minmax_%d.txt", i);

      FILE *f = fopen(filename, "r");

      if (f) {
        fscanf(f, "%d %d", &min, &max);
        fclose(f);
        unlink(filename);
      }
    } else {
      close(pipes[i][1]);

      struct MinMax child_result;

      if (read(
              pipes[i][0],
              &child_result,
              sizeof(child_result)
          ) == sizeof(child_result)) {
        min = child_result.min;
        max = child_result.max;
      }

      close(pipes[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time =
      (finish_time.tv_sec - start_time.tv_sec) * 1000.0;

  elapsed_time +=
      (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);

  return 0;
}