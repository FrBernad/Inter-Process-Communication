/*
● DEBE recibir el/los paths de los archivos a procesar y DEBE iniciar el programa
correspondiente para procesarlos (minisat).
● DEBE enviar la información relevante del procesamiento (ver proceso vista) al
proceso aplicación.
● NO DEBE volcar el resultado de minisat a un archivo en disco, para luego leerlo
desde el esclavo, DEBERÁ recibir el output de minisat utilizando algún mecanismo
de IPC más sofisticado.
● No es necesario escribir un parser del output de minisat, se pueden utilizar
comandos como grep, sed, awk, etc.
○ Hint: popen
○ Hint: grep -o -e "Number of.*[0-9]\+" -e "CPU time.*" -e
".*SATISFIABLE"
*/

#define _POSIX_C_SOURCE 2
#define ERROR_CODE 1
#define MAX_TASKS_LENGTH 4096 - 1
#define ERROR_MANAGER(ERROR_STRING) \
    do {                            \
        perror(ERROR_STRING);       \
        exit(EXIT_FAILURE);         \
    } while (0)

#define SAT_SOLVER "minisat"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static void processTask(char *task);

int main(int argc, char const *argv[]) {
    // Disable buffering on stdout
    if (setvbuf(stdout, NULL, _IONBF, 0)) {
        //check error
    }
    for (size_t i = 0; i < argc; i++) {
        processTask((char *)argv[i]);
    }

    char task[MAX_TASKS_LENGTH + 1] = {0};
    ssize_t count;

    while ((count = read(STDIN_FILENO, task, MAX_TASKS_LENGTH)) != 0) {
        if (count == -1) {
            ERROR_MANAGER("slave > main > read input");
        }
        task[count] = 0;
        processTask(task);
    }

    return 0;
}

static void processTask(char *tasks) {
    char *task = strtok(tasks, "\t");

    FILE *outputStream;
    char command[MAX_TASKS_LENGTH + 1];
    char output[MAX_TASKS_LENGTH + 1];
    int count;

    while (task != NULL) {
        sprintf(command, "%s %s | grep -o -e \"Number of.*[0 - 9]\\+\" -e \"CPU time.*\" -e \".*SATISFIABLE\"", SAT_SOLVER, task);

        if ((outputStream = popen(command, "r")) == NULL)
            ERROR_MANAGER("slave>processTask>popen");

        count = fread(output, sizeof(char), MAX_TASKS_LENGTH, outputStream);

        if (ferror(outputStream))
            ERROR_MANAGER("slave>processTask>fread");

        printf("%s\t", output);

        output[count] = 0;

        pclose(outputStream);

        task = strtok(NULL, "\t");
    }
}
