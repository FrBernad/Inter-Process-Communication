// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
DEBE recibir por línea de comando los archivos a procesar, por ejemplo:
    $ solve files
● DEBE iniciar los procesos esclavos.
● DEBE distribuir una cierta cantidad (significativamente menor que el total) de
archivos entre los esclavos, por ejemplo, si se requiere procesar 100 archivos entre
5 esclavos se pueden distribuir 2 archivos por esclavo inicialmente, es decir 10
archivos.
● Cuando un esclavo se libera (TERMINA TODAS SUS TAREAS, PENDING TASKS <=0), la aplicación le DEBE enviar UN nuevo archivo para
procesar.
● DEBE recibir el resultado del procesamiento de cada archivo y DEBE agregarlo a un
buffer POR ORDEN DE LLEGADA .
● Cuando inicia, DEBE esperar 2 segundos a que aparezca un proceso vista, si lo
hace le comparte el buffer de llegada.
● SOLO debe imprimir por stdout el input que el proceso vista requiera para
conectarse al buffer compartido (ver más adelante).
● Termina cuando todos los archivos estén procesados.
● DEBE guardar el resultado en el archivo resultado, aparezca el proceso de vista o
no.
*/

/*
0: stdin
1: stdout
2: stderr

fd[0] read
fd[1] write
*/

// #FIXME: fijarse cuando poner < o == -1

#define _XOPEN_SOURCE 500  //for ftruncate

#include <errno.h>
#include <fcntl.h>
#include <limits.h>  //PIPE_BUF NO APARECE :C
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SLAVES_COUNT 5
#define READ 0
#define WRITE 1
#define MAX_OUTPUT_LENGTH 4096
#define MIN_INIT_TASKS 2
#define SLAVE_FILENAME "slave"
#define SHR_MEM_NAME "/shm_buffer"
#define SEM_NAME "/shr_sem"

#define ERROR_MANAGER(ERROR_STRING)                                                            \
    do {                                                                                       \
        fprintf(stderr, "Error in %s, line %d : %s\n", ERROR_STRING, __LINE__, strerror(errno)); \
        exit(EXIT_FAILURE);                                                                    \
    } while (0)

typedef struct {
    pid_t pid;
    int inputFD;
    int outputFD;
    int pendingTasks;
} t_slave;

static void initSlaves(t_slave slaves[SLAVES_COUNT], char *tasks[], size_t *pendingTasks, size_t *taskIndex, size_t *workingSlaves);
static void assignTask(t_slave *slave, char const *tasks[], size_t *pendingTasks, size_t *taskIndex);
static void terminateSlaves(t_slave *slaves, size_t workingSlaves);
static void initShm(char **shmBase, int *shmFD, size_t size);
static void terminateShm(char *shmBase, int shmFD, size_t size);
static void sendTaskInfo(char *tasksOutput, size_t recievedTasks, sem_t *sem, size_t *shmIndex, char *shmBase);
static void terminateSem(sem_t *sem);

/*
        send parsed data
    --- R--------------W
MASTER                   SLAVE
   |    W--------------R
   |     send file path
   | 
   |      semaforo R/W
    ---  SHR_MEM BUFFER  --- VISTA
*/

int main(int argc, char const *argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "Wrong number of parameters, expected at least one valid file path name\n");
        exit(EXIT_FAILURE);
    }

    if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
          ERROR_MANAGER("solve > main > setvbuff");

    size_t totalTasks = argc - 1, processedTasks = 0, pendingTasks = totalTasks, taskIndex = 0;

    char *shmBase;
    int shmFD;
    size_t shmIndex = 0;

    initShm(&shmBase, &shmFD, totalTasks * MAX_OUTPUT_LENGTH);
    sem_t *sem = sem_open(SEM_NAME, O_EXCL|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, 0);

    if (sem == SEM_FAILED)
        ERROR_MANAGER("solve > main > sem_open");
    
    printf("%ld",totalTasks);

    sleep(2);

    size_t workingSlaves = SLAVES_COUNT;

    if (SLAVES_COUNT > totalTasks)
        workingSlaves = totalTasks;

    t_slave slaves[workingSlaves];

    char const **tasks = argv + 1;

    initSlaves(slaves, (char **)tasks, &pendingTasks, &taskIndex, &workingSlaves);

    fd_set readfds;

    while (processedTasks < totalTasks) {
        //clear the read set
        FD_ZERO(&readfds);

        int maxfd = -1;  //max fd for select call

        //add slaves fds to read set
        for (size_t i = 0; i < workingSlaves; i++) {
            int readfd = slaves[i].outputFD;

            //add to set
            FD_SET(readfd, &readfds);

            //update if necessary maxfd
            if (readfd > maxfd) {
                maxfd = readfd;
            }
        }

        //wait until slave can process another file
        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        //check select error
        if (activity < 0)
            ERROR_MANAGER("solve > main > Select error");

        //check which tasks are over and send others
        char tasksOutput[MAX_OUTPUT_LENGTH + 1] = {0};  //+1 for /0

        for (size_t i = 0; i < workingSlaves; i++) {
            int readfd = slaves[i].outputFD;

            // save parsed data to buffer
            if (FD_ISSET(readfd, &readfds)) {
                ssize_t count;

                if ((count = read(readfd, tasksOutput, MAX_OUTPUT_LENGTH)) < 0)
                    ERROR_MANAGER("solve > main > Read error");

                if (count != 0) {  // read != EOF
                    tasksOutput[count] = 0;

                    size_t recievedTasks = 0;

                    for (char *aux = tasksOutput; (aux = strchr(aux, '\t')) != NULL; aux++) {
                        slaves[i].pendingTasks--;
                        processedTasks++;
                        recievedTasks++;
                    }

                    sendTaskInfo(tasksOutput, recievedTasks, sem, &shmIndex, shmBase);

                    //assign, if possible, new task
                    if (slaves[i].pendingTasks <= 0 && pendingTasks > 0)
                        assignTask(&slaves[i], tasks, &pendingTasks, &taskIndex);
                }
            }
        }
    }

    terminateSlaves(slaves, workingSlaves);

    terminateSem(sem);

    terminateShm(shmBase, shmFD,totalTasks * MAX_OUTPUT_LENGTH);

    return 0;
}

static void initSlaves(t_slave slaves[SLAVES_COUNT], char *tasks[], size_t *pendingTasks, size_t *taskIndex, size_t *workingSlaves) {
    pid_t pid;
    int slaveMaster[2], masterSlave[2], initTasks = 1;

    if (*pendingTasks >= (MIN_INIT_TASKS * (*workingSlaves)))
        initTasks = MIN_INIT_TASKS;

    char *childArgs[initTasks + 2];

    for (size_t i = 0; i < *workingSlaves; i++) {

          slaves[i].pendingTasks = 0;
          //create master-slave pipe
          if (pipe(slaveMaster) < 0)
                ERROR_MANAGER("solve > initSlave > creating slave-master pipe");

          //create pipe
          if (pipe(masterSlave) < 0)
                ERROR_MANAGER("solve > initSlave > creating master-slave pipe");

          slaves[i].outputFD = slaveMaster[READ];
          slaves[i].inputFD = masterSlave[WRITE];

          //create slave
          if ((pid = fork()) == 0) {
                //close uncorresponding fds slaves and dup

                if (dup2(masterSlave[READ], STDIN_FILENO) < 0)
                      ERROR_MANAGER("solve > initSlave > dupping slave pipe");

                if (dup2(slaveMaster[WRITE], STDOUT_FILENO) < 0)
                      ERROR_MANAGER("solve > initSlave > dupping slave pipe");

                //closed dupped fds
                if (close(masterSlave[READ]) < 0)
                      ERROR_MANAGER("solve > initSlave > closing slave fd");

                if (close(slaveMaster[WRITE]) < 0)
                      ERROR_MANAGER("solve > initSlave > closing slave fd");

                //closed unnecessary fds
                if (close(masterSlave[WRITE]) < 0)
                      ERROR_MANAGER("solve > initSlave > closing slave fd");

                if (close(slaveMaster[READ]) < 0)
                      ERROR_MANAGER("solve > initSlave > closing slave fd");

                size_t j;
                for (j = 1; j < initTasks + 1; j++)
                      childArgs[j] = tasks[(*taskIndex)++];

                childArgs[0] = SLAVE_FILENAME;
                childArgs[j] = NULL;

                //excecute slave
                if (execv(childArgs[0], childArgs) < 0)
                      ERROR_MANAGER("solve > initSlave > exec slave");

        } else if (pid == -1)
            ERROR_MANAGER("solve > initSlave > slave fork ");

        slaves[i].pendingTasks += initTasks;
        slaves[i].pid = pid;
        *pendingTasks -= initTasks;
        *taskIndex += initTasks;

        //closed unnecessary fds
        if (close(masterSlave[READ]) < 0)
            ERROR_MANAGER("solve > initSlave > closing master fd");

        if (close(slaveMaster[WRITE]) < 0)
            ERROR_MANAGER("solve > initSlave > closing master fd");
    }
}

static void initShm(char **shmBase, int *shmFD, size_t size) {
    *shmFD = shm_open(SHR_MEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);  //0666
   
    if (*shmFD == -1)
        ERROR_MANAGER("solve > initShm > shm_open");

    if (ftruncate(*shmFD, size) == -1)
        ERROR_MANAGER("solve > initShm > ftruncate");

    *shmBase = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *shmFD, 0);

    if (*shmBase == MAP_FAILED)
        ERROR_MANAGER("solve > initShm > mmap");
}

static void assignTask(t_slave *slave, char const *tasks[], size_t *pendingTasks, size_t *taskIndex) {
    size_t len = strlen(tasks[*taskIndex]) + 2;  // for /t and /0
    char tasksStr[len];

    if (sprintf(tasksStr, "%s\t", tasks[*taskIndex]) < 0)
        ERROR_MANAGER("solve > assignTask > sprintf");

    if (write(slave->inputFD, tasksStr, len) < 0)
        ERROR_MANAGER("solve > assignTask > write");

    (*taskIndex)++;
    (*pendingTasks)--;
    (slave->pendingTasks)++;
}

static void sendTaskInfo(char *tasksOutput, size_t recievedTasks, sem_t *sem, size_t *shmIndex, char *shmBase) {
    size_t tasksOutputSize = strlen(tasksOutput);

    memcpy(shmBase + *shmIndex, tasksOutput, tasksOutputSize);
    *shmIndex += tasksOutputSize;

    for (size_t i = 0; i < recievedTasks; i++) {
        if (sem_post(sem) == -1)
            ERROR_MANAGER("solve > main > sendTaskInfo");
    }
}

static void terminateShm(char *shmBase, int shmFD, size_t size) {
    if (close(shmFD) == -1)
        ERROR_MANAGER("solve > terminateShm > close");

    if (shm_unlink(SHR_MEM_NAME) == -1)
        if (errno != ENOENT)
            ERROR_MANAGER("solve > terminateShm > shm_unlink");

    if (munmap(shmBase, size) == -1)
        ERROR_MANAGER("solve > terminateShm > munmap");
}

static void terminateSem(sem_t *sem) {
    if (sem_close(sem) == -1)
        ERROR_MANAGER("solve > terminateSem > close");

    if (sem_unlink(SEM_NAME) == -1)
        if (errno != ENOENT)
            ERROR_MANAGER("solve > terminateSem > sem_unlink");
}


static void terminateSlaves(t_slave *slaves, size_t workingSlaves) {
    for (size_t i = 0; i < workingSlaves; i++) {
        //closed fds
        if (close(slaves[i].inputFD) < 0)
            ERROR_MANAGER("solve > terminateSlaves > closing pipe");

        //closed fds
        if (close(slaves[i].outputFD) < 0)
            ERROR_MANAGER("solve > terminateSlaves > closing pipe");
    }

    for (size_t i = 0; i < workingSlaves; i++) {
        if (wait(NULL) < 0)
            ERROR_MANAGER("solve > terminateSlaves > waiting for slave to finish\n");
    }
}