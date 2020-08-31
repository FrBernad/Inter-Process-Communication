/*
DEBE recibir por línea de comando los archivos a procesar, por ejemplo:
    $ solve files
● DEBE iniciar los procesos esclavos.
● DEBE distribuir una cierta cantidad (significativamente menor que el total) de
archivos entre los esclavos, por ejemplo, si se requiere procesar 100 archivos entre
5 esclavos se pueden distribuir 2 archivos por esclavo inicialmente, es decir 10
archivos.
● Cuando un esclavo se libera, la aplicación le DEBE enviar UN nuevo archivo para
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

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define SLAVES_COUNT 2
#define ERROR_CODE 1
#define READ 0
#define WRITE 1
#define MAX_TASK_LENGTH 50
#define MAX_OUTPUT_LENGTH 50
#define INIT_TASKS 2
#define MAX_TASKS_PER_SLAVE 2
#define SLAVE_FILENAME "slave"

#define ERROR_MANAGER(ERROR_STRING)                            \
                                do {                           \
                                       perror(ERROR_STRING); \
                                       exit(ERROR_CODE);       \
                                } while (0)

typedef struct {
pid_t pid;
int inputFD;
int outputFD;
int pendingTasks;
}
t_slave;

static void initSlaves(t_slave slaves[SLAVES_COUNT], char *tasks[], size_t *pendingTasks, size_t *taskIndex);
static void assignTask(t_slave *slave, char const *tasks[], size_t *pendingTasks, size_t *taskIndex);
static void terminateSlaves(t_slave slaves[SLAVES_COUNT]);

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
        ERROR_MANAGER("Wrong number of parameters, expected at least one valid file name\n");
    }

    t_slave slaves[SLAVES_COUNT];
    size_t totalTasks = argc - 1, processedTasks = 0, pendingTasks = totalTasks, taskIndex;
    char const **tasks = argv + 1;

    initSlaves(slaves,(char**)tasks, &pendingTasks, &taskIndex);

    FILE *outputFile = fopen("output.txt", "a+");

    fd_set readfds;

    while (processedTasks < totalTasks) {
        //clear the read set
        FD_ZERO(&readfds);

        int maxfd = -1;  //max fd for select call

        //add slaves fds to read set
        for (size_t i = 0; i < SLAVES_COUNT; i++) {
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
        if (activity < 0) {
            ERROR_MANAGER("Select error\n");
        }

        //check which tasks are over and send others
        char tasksOutput[MAX_OUTPUT_LENGTH] = {0};

        for (size_t i = 0; i < SLAVES_COUNT; i++) {
            int readfd = slaves[i].outputFD;

            // save parsed data to buffer
            if (FD_ISSET(readfd, &readfds)) {
                ssize_t count;

                if ((count = read(readfd, tasksOutput, MAX_OUTPUT_LENGTH)) < 0) {
                    ERROR_MANAGER("Error reading data from slave\n");
                }

                // Get the first task output
                char *output = strtok(tasksOutput, "\t");

                // Walk through other task outputs
                while (output != NULL) {
                    slaves[i].pendingTasks--;
                    processedTasks++;
                    fwrite(output,sizeof(char),strlen(output),outputFile);
                    output = strtok(NULL, output);
                }

                //assign, if possible, new task
                if (slaves[i].pendingTasks <= 0 && pendingTasks > 0 && slaves[i].pendingTasks < MAX_TASKS_PER_SLAVE) {
                    assignTask(&slaves[i], tasks, &pendingTasks, &taskIndex);
                }
            }
        }
    }

    terminateSlaves(slaves);

    fclose(outputFile);

    return 0;
}

static void terminateSlaves(t_slave slaves[SLAVES_COUNT]) {

    for (size_t i = 0; i < SLAVES_COUNT; i++) {
        //closed fds
        if (close(slaves[i].inputFD)<0) {
            ERROR_MANAGER("Error closing pipe\n");
        }
        //closed fds
        if (close(slaves[i].outputFD)<0) {
            ERROR_MANAGER("Error closing pipe\n");
        }
    }

    for (size_t i = 0; i < SLAVES_COUNT; i++) {
        if (wait(NULL) < 0) {
            ERROR_MANAGER("Error waiting for slave to finish\n");
        }
    }
}

static void initSlaves(t_slave slaves[SLAVES_COUNT], char *tasks[], size_t *pendingTasks, size_t *taskIndex) {
    pid_t pid;
    int slaveMaster[2], masterSlave[2];

    for (size_t i = 0; i < SLAVES_COUNT; i++) {

        //create master-slave pipe
        if (pipe(slaveMaster) < 0) {
            ERROR_MANAGER("Error creating slave-master pipe\n");
        }

        //create pipe
        if (pipe(masterSlave) < 0) {
            ERROR_MANAGER("Error creating master-slave pipe\n");
        }

        slaves[i].outputFD = slaveMaster[READ];
        slaves[i].inputFD = masterSlave[WRITE];
            
        //create slave
        if ((pid = fork()) == 0) {
            //close uncorresponding fds slaves and dup

            if (dup2(masterSlave[READ], STDIN_FILENO)<0) {
                ERROR_MANAGER("Error dupping pipe\n");
            }

            if (dup2(slaveMaster[WRITE],STDOUT_FILENO)<0) {
                // printf("mmap failed: %s", strerror(errno));
                ERROR_MANAGER("Error dupping pipe\n");
            }
                
            //closed dupped fds
            if (close(masterSlave[READ])<0) {
                ERROR_MANAGER("Error closing pipe\n");
            }

            if (close(slaveMaster[WRITE])<0) {
                ERROR_MANAGER("Error closing pipe\n");
            }

            //closed unnecessary fds
            if (close(masterSlave[WRITE])<0) {
                ERROR_MANAGER("Error closing pipe\n");
            }

            if (close(slaveMaster[READ])<0) {
                ERROR_MANAGER("Error closing pipe\n");
            }

            char *initTasks[INIT_TASKS+1];

            size_t i = 0;

            for (; i < INIT_TASKS; i++) {
                fprintf(stderr,"Adding task %s\n",tasks[*taskIndex]);
                initTasks[i] = tasks[(*taskIndex)++];
                (*pendingTasks)--;
            }

            initTasks[i]=NULL;

            //excecute slave
            if (execv(SLAVE_FILENAME, initTasks) < 0) {
                ERROR_MANAGER("Error when attempting to exec slave, aborting\n");
            }

        } else if (pid == -1) {
            ERROR_MANAGER("Error when excecuting fork\n");
        }

        slaves[i].pendingTasks+=INIT_TASKS;
        slaves[i].pid = pid;
        (*taskIndex)+=2;
        
        //closed unnecessary fds
        if (close(masterSlave[READ])<0) {
            ERROR_MANAGER("Error closing pipe\n");
        }

        if (close(slaveMaster[WRITE])<0) {
            ERROR_MANAGER("Error closing pipe\n");
        }
    }
}

static void assignTask(t_slave *slave, char const *tasks[], size_t *pendingTasks, size_t *taskIndex) {
    if (write(slave->inputFD, tasks[(*taskIndex)++], MAX_TASK_LENGTH) < 0) {
        ERROR_MANAGER("Error assigning task\n");
    }

    (*pendingTasks)--;
    slave->pendingTasks++;

    return;
}