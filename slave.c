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

#define ERROR_CODE 1

int main(int argc, char const *argv[]){

    char fileName[PIPE_BUF] = {0};
    
    while(read(fd,fileName,PIPE_BUF-1)>=0){
        proceasr
    }

    return 0;
}
