// Sebastian Aristizabal y Alejandro Arias Ortiz
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define PATH_SIZE 256

const char *default_path[] = { "/bin", NULL }; // Ruta de búsqueda inicial
char *search_path[PATH_SIZE]; // Variable para almacenar rutas de búsqueda

void initialize_path() {
    search_path[0] = "/bin"; // Inicializa la ruta
}

void parse_input(char *input, char **args) {
    char *token;
    int i = 0;

    token = strtok(input, " \n"); // Divide la entrada por espacios
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \n");
    }
    args[i] = NULL; // Finaliza la lista de argumentos
}

int execute_command(char **args) {
    pid_t pid = fork();
    if (pid == 0) { // Proceso hijo
        if (execvp(args[0], args) < 0) { // Ejecuta el comando
            perror("An error has occurred");
            exit(1); // Salir si hay error
        }
    } else if (pid < 0) {
        perror("An error has occurred");
    } else { // Proceso padre
        wait(NULL); // Esperar a que el hijo termine
    }
    return 0;
}

void change_directory(char *path) {
    if (chdir(path) != 0) {
        perror("An error has occurred");
    }
}

void shell_loop() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    while (1) {
        printf("wish> "); // Prompt del shell
        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            break; // Salir en EOF
        }

        if (strcmp(input, "exit\n") == 0) {
            break; // Salir del shell
        } else if (strncmp(input, "cd ", 3) == 0) {
            change_directory(input + 3); // Cambiar de directorio
        } else {
            parse_input(input, args); // Parsear el comando
            execute_command(args); // Ejecutar el comando
        }
    }
}

int main(int argc, char *argv[]) {
    initialize_path(); // Inicializar ruta
    shell_loop(); // Iniciar bucle del shell
    return 0;
}

