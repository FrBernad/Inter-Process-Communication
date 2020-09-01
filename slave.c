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
#define MAX_TASKS_LENGTH 4096-1

static void processTask(char * task);

int main(int argc, char const *argv[]){

    // Disable buffering on stdout
    if(setvbuf(stdout, NULL, _IONBF, 0)){
    //check error
    }
    for (size_t i = 0; i < argc; i++)
    {
        processTask((char*)argv[i]);
    }
    
    char task[MAX_TASKS_LENGTH]={0};
    ssize_t count;

    while((count=read(STDIN_FILENO,task,MAX_TASKS_LENGTH))!=0){
        if(count==-1){
            //handle error
        }
        task[count]=0;
        processTask(task);
    }

    return 0;
}

static void processTask(char * task) {

    fprintf(stderr,"processing task %s \n", task);
    printf("%s\t", task);
}
