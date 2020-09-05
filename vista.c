// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_OUTPUT_LENGTH 4096
#define MAX_TOTAL_FILES 4000
#define SHR_MEM_NAME "/shm_buffer"
#define SEM_NAME "/shr_sem"

#define ERROR_MANAGER(ERROR_STRING)                                                                  \
      do {                                                                                           \
            fprintf(stderr, "Error in %s, line %d : %s\n", ERROR_STRING, __LINE__, strerror(errno)); \
            exit(EXIT_FAILURE);                                                                      \
      } while (0)

static void initShm(char **shmBase, int *shmFD, size_t size);
static void terminateShm(char *shmBase, int shmFD, size_t size);
static void terminateSem(sem_t *sem);
static void outputTasks(char *shmBase, sem_t *sem, size_t totalTasks);

int main(int argc, char const *argv[]) {
      size_t totalTasks = 0;

      if (argc - 1 == 0) {  //stdin
            char totalTasksStr[MAX_TOTAL_FILES + 1];
            ssize_t bytesRead = 0;

            if ((bytesRead = read(STDIN_FILENO, totalTasksStr, MAX_TOTAL_FILES)) == -1)
                  ERROR_MANAGER("vista > main > read total files");
            totalTasksStr[bytesRead] = 0;

            totalTasks = (size_t)atoi(totalTasksStr);

      } else if (argc - 1 == 1)
            totalTasks = atoi(argv[1]);
      else {
            fprintf(stderr, "Wrong number of parameters, expected info from solve or to be piped\n");
            exit(EXIT_FAILURE);
      }

      char *shmBase;
      int shmFD;

      initShm(&shmBase, &shmFD, totalTasks * MAX_OUTPUT_LENGTH);

      sem_t *sem = sem_open(SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, 0);

      if (sem == SEM_FAILED)
            ERROR_MANAGER("vista > main > sem_open");

      outputTasks(shmBase, sem, totalTasks);

      terminateSem(sem);

      terminateShm(shmBase, shmFD, totalTasks * MAX_OUTPUT_LENGTH);

      return 0;
}

static void outputTasks(char *shmBase, sem_t *sem, size_t totalTasks) {
      char *currentTask = shmBase, *nextTask;

      for (size_t i = 0; i < totalTasks; i++) {
            if (sem_wait(sem) == -1)
                  ERROR_MANAGER("vista > outputTasks > sem_wait");

            if ((nextTask = strchr(currentTask, '\t')) == NULL)
                  ERROR_MANAGER("vista > outputTasks > strchr");  //#TODO: VER Q ONDA

            *nextTask = '\0';
            nextTask++;

            printf("%s\n", currentTask);

            currentTask = nextTask;
      }
}

static void initShm(char **shmBase, int *shmFD, size_t size) {
      *shmFD = shm_open(SHR_MEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);  //0666

      if (*shmFD == -1)
            ERROR_MANAGER("vista > initShm > shm_open");

      *shmBase = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *shmFD, 0);

      if (*shmBase == MAP_FAILED)
            ERROR_MANAGER("vista > initShm > mmap");
}

static void terminateShm(char *shmBase, int shmFD, size_t size) {
      if (close(shmFD) == -1)
            ERROR_MANAGER("vista > terminateShm > close");

      if (shm_unlink(SHR_MEM_NAME) == -1)
            if (errno != ENOENT)
                  ERROR_MANAGER("vista > terminateShm > shm_unlink");

      if (munmap(shmBase, size) == -1)
            ERROR_MANAGER("vista > terminateShm > munmap");
}

static void terminateSem(sem_t *sem) {
      if (sem_close(sem) == -1)
            ERROR_MANAGER("vista > terminateSem > close");

      if (sem_unlink(SEM_NAME) == -1)
            if (errno != ENOENT)
                  ERROR_MANAGER("vista > terminateSem > sem_unlink");
}