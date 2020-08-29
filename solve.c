/*
DEBE recibir por línea de comando los archivos a procesar, por ejemplo:
    $ solve files/*
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#define SLAVES_COUNT 5
#define ERROR_CODE 1
#define READ 0
#define WRITE 1
#define MASTER_SLAVE 0
#define SLAVE_MASTER 1

static int initSlaves(int pipes[SLAVES_COUNT][2][2]);

int main(int argc, char const *argv[]) {

    if (argc <= 1) {
        fprintf(stderr,
                "Wrong number of params:"
                "\n recieved: %d"
                "\n expected at least one valid directory or file name\n",
                argc - 1);

        exit(ERROR_CODE);
    }

    int pipes[SLAVES_COUNT][2][2];

    if (initSlaves(pipes) == ERROR_CODE) {
        fprintf(stderr, "Error in slaves initilization, aborting \n");
        exit(ERROR_CODE);
    }

    // const char ** fileNames = argv+1;
    // int fileCount = argc-1, error=0;

    // for (size_t i = 0; i < fileCount & !error; i++)
    // {

    // }

    fd_set readfds, writefds, exceptfds;

    while (fileCount > 0) {

        //clear the slave-master read set
        FD_ZERO(&readfds);

        int maxfd = -1;  //max fd for select call

        //add slaves fds to read set
        for (size_t i = 0; i < SLAVES_COUNT; i++) {
            int currentfd = pipes[i][SLAVE_MASTER][READ];

            //add to set
            FD_SET(currentfd, &readfds);

            //update if necessary maxfd
            if (currentfd > maxfd) {
                maxfd = currentfd;
            }
        }

        //wait until slave can process another file
        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        //check select error
        if (activity < 0) {
            fprintf(stderr, "Select error\n");
            exit(ERROR_CODE);
        }

        //if slave is available and there are still files to process, send new file
        for (size_t i = 0; i < SLAVES_COUNT; i++) {
            int currentfd = pipes[i][SLAVE_MASTER][READ];

            //check if fd is 
            if(FD_ISSET(currentfd, &readfds)){
                
            }
            
        }
    }

    return 0;
}

static int initSlaves(int pipes[SLAVES_COUNT][2][2]) {
    int retVal = 0, pid;

    for (size_t i = 0; i < SLAVES_COUNT; i++) {
        //create master-slave pipe
        if (pipe(pipes[i][SLAVE_MASTER]) < 0) {
            fprintf(stderr, "Error creating master-slave pipe\n");
            retVal = ERROR_CODE;
            break;
        }

        //create master-slave pipe
        if (pipe(pipes[i][MASTER_SLAVE]) < 0) {
            fprintf(stderr, "Error creating slave-master pipe\n");
            retVal = ERROR_CODE;
            break;
        }

        //create slave
        if ((pid = fork()) == 0) {
            //close uncorresponding pipes
            for (size_t j = 0; j < i; i++) {
                close(pipes[j][MASTER_SLAVE][READ]);  //CHEKEAR ERRROR DE CLOSE
                close(pipes[j][MASTER_SLAVE][WRITE]);
                close(pipes[j][SLAVE_MASTER][READ]);
                close(pipes[j][SLAVE_MASTER][WRITE]);
            }

            //create fds to pass as argument to slave
            char fds[3]={0};
            sprintf(fds, "%d%d", pipes[i][SLAVE_MASTER][READ], pipes[i][SLAVE_MASTER][WRITE]);

            //excecute slave
            char *args[] = {"slave",fds, NULL};
            if (execv(args[0], args) < 0) {
                fprintf(stderr, "Error when attempting to exec slave, aborting\n");
                exit(-1);
            }
        } else if (pid == -1) {
            retVal = ERROR_CODE;
            break;
        }
    }

    return retVal;
}
