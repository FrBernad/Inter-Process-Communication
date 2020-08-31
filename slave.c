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

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define ERROR_CODE 1
#define MAX_TASKS_LENGTH 2000

static void processTask(char * task);

int main(int argc, char const *argv[]){

    // Disable buffering on stdout
    if(setvbuf(stdout, NULL, _IONBF, 0)){
    //check error
    }

    for (size_t i = 1; i < argc; i++)
    {
        processTask((char*)argv[i]);
    }
    
    char tasks[MAX_TASKS_LENGTH]={0};
 
    while(read(STDIN_FILENO,tasks,MAX_TASKS_LENGTH)>0){
        processTask(tasks);
    }

    return 0;
}

static void processTask(char * tasks) {

    char *task = strtok(tasks, "\t");
    fprintf(stderr,"processing task %s \n", task);

    // Walk through other task outputs
    while (task != NULL) {
        task = strtok(NULL, tasks);
    }
}
