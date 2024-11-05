// Sebastian Aristizabal y Alejandro Arias Ortiz
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[]) {
    // Verificar que se ha pasado al menos un comando
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return 1;
    }

    pid_t pid;
    struct timeval start, end;

    // Obtener tiempo inicial
    gettimeofday(&start, NULL);

    // Crear un nuevo proceso
    pid = fork();
    if (pid < 0) {
        perror("Fork falló"); // Manejo de error en fork
        return 1;
    }

    if (pid == 0) {
        // Proceso hijo: ejecutar el comando
        execvp(argv[1], &argv[1]); // Ejecutar el comando
        perror("Exec falló"); // Si execvp falla
        exit(1); // Terminar el hijo en caso de error
    } else {
        // Proceso padre: esperar a que el hijo termine
        wait(NULL); // Esperar al proceso hijo
    }

    // Obtener tiempo final
    gettimeofday(&end, NULL);

    // Calcular el tiempo transcurrido
    double elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Mostrar el tiempo transcurrido
    printf("Elapsed time: %.5f\n", elapsedTime);

    return 0;
}

