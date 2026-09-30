#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
  pid_t child_pid = fork();

  if (child_pid < 0) {
    perror("fork");
    return 1;
  }

  if (child_pid == 0) {
    // Дочерний процесс сразу завершается.
    printf("Child: PID = %d, parent PID = %d\n", getpid(), getppid());
    printf("Child: exiting...\n");
    fflush(stdout);

    exit(0);
  }

  // Родительский процесс НЕ вызывает wait сразу.
  // Поэтому завершившийся дочерний процесс станет zombie.
  printf("Parent: PID = %d\n", getpid());
  printf("Parent: child PID = %d\n", child_pid);
  printf("Parent: sleeping for 30 seconds without wait().\n");
  printf("Check child process with:\n");
  printf("ps -o pid,ppid,state,stat,cmd -p %d\n", child_pid);
  fflush(stdout);

  sleep(30);

  // Забираем информацию о завершившемся дочернем процессе.
  waitpid(child_pid, NULL, 0);

  printf("Parent: waitpid() called. Zombie process removed.\n");

  return 0;
}