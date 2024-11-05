// Sebastian Aristizabal y Alejandro Arias Ortiz
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64
#define MAX_PATHS 64

// Mensaje de error estándar
char error_message[30] = "An error has occurred\n";

// Ruta de búsqueda inicial
char *path[MAX_PATHS];

// Declaración de funciones
void print_error();
int execute_builtin(char **args);
void execute_command(char **args);
int handle_redirection(char **args);
void handle_parallel_commands(char *input);
char *preprocess_input(char *input);

// Variable global para indicar modo batch
int batch_mode = 0;

// Función para mostrar error
void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Función principal
int main(int argc, char *argv[]) {
    char input[BUFFER_SIZE];
    FILE *input_stream = stdin;

    // Inicializar path[]
    path[0] = strdup("/bin");
    path[1] = NULL;

    // Verificar argumentos de entrada para modo batch
    if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (!input_stream) {
            print_error();
            exit(1);
        }
        batch_mode = 1; // Estamos en modo batch
    } else if (argc > 2) {
        print_error();
        exit(1);
    }

    while (1) {
        // Imprimir el prompt solo en modo interactivo
        if (!batch_mode) {
            printf("wish> ");
        }

        // Leer entrada
        if (fgets(input, BUFFER_SIZE, input_stream) == NULL) {
            break;
        }

        // Eliminar salto de línea y retorno de carro
        input[strcspn(input, "\n")] = '\0';
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\r') {
            input[len - 1] = '\0';
        }

        // Manejar comandos paralelos
        handle_parallel_commands(input);
    }

    if (input_stream != stdin) {
        fclose(input_stream);
    }

    // Liberar memoria de path[]
    for (int i = 0; path[i] != NULL; i++) {
        free(path[i]);
    }

    return 0;
}

// Función para reemplazar '&' por ' & '
char *preprocess_input(char *input) {
    size_t len = strlen(input);
    char *modified_input = malloc(len * 3 + 1); // Tamaño máximo posible
    if (modified_input == NULL) {
        print_error();
        exit(1);
    }
    int idx = 0;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '&') {
            modified_input[idx++] = ' ';
            modified_input[idx++] = '&';
            modified_input[idx++] = ' ';
        } else {
            modified_input[idx++] = input[i];
        }
    }
    modified_input[idx] = '\0';
    return modified_input;
}

void handle_parallel_commands(char *input) {
    char *commands[MAX_ARGS];
    int num_commands = 0;

    // Preprocesar la entrada
    char *modified_input = preprocess_input(input);

    // Dividir la entrada en comandos separados por '&'
    char *token = strtok(modified_input, "&");
    while (token != NULL && num_commands < MAX_ARGS - 1) {
        // Eliminar espacios iniciales y finales
        while (*token == ' ' || *token == '\t') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) *end-- = '\0';

        if (*token != '\0') {
            commands[num_commands++] = token;
        }
        token = strtok(NULL, "&");
    }
    commands[num_commands] = NULL;

    pid_t pids[MAX_ARGS];
    int pid_index = 0;

    // Ejecutar cada comando
    for (int i = 0; i < num_commands; i++) {
        char *args[MAX_ARGS];
        int j = 0;

        // Tokenizar argumentos, manejando espacios variables
        token = strtok(commands[i], " \t");
        while (token != NULL && j < MAX_ARGS - 1) {
            args[j++] = token;
            token = strtok(NULL, " \t");
        }
        args[j] = NULL;

        // Verificar si hay un comando
        if (args[0] != NULL) {
            // Comandos internos
            if (strcmp(args[0], "exit") == 0) {
                if (args[1] != NULL) {
                    print_error();
                    continue;
                }
                // Liberar memoria antes de salir
                for (int k = 0; path[k] != NULL; k++) {
                    free(path[k]);
                }
                free(modified_input);
                exit(0);
            } else if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "path") == 0) {
                int result = execute_builtin(args);
                if (result == -1) {
                    print_error();
                }
                continue;
            }

            // Comandos externos
            pid_t pid = fork();
            if (pid < 0) {
                print_error();
            } else if (pid == 0) {
                // Proceso hijo
                if (handle_redirection(args) == -1) {
                    print_error();
                    exit(1);
                }
                execute_command(args);
                exit(0); // Salir después de ejecutar el comando
            } else {
                // Proceso padre
                pids[pid_index++] = pid;
            }
        }
    }

    // Esperar a que terminen todos los procesos hijos
    for (int i = 0; i < pid_index; i++) {
        waitpid(pids[i], NULL, 0);
    }

    free(modified_input);
}

// Función para ejecutar comandos integrados
int execute_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
            return -1;
        }
        if (chdir(args[1]) != 0) {
            return -1;
        }
        return 0;
    } else if (strcmp(args[0], "path") == 0) {
        // Liberar path existente
        for (int i = 0; path[i] != NULL; i++) {
            free(path[i]);
            path[i] = NULL;
        }
        // Construir nuevo path
        int index = 0;
        for (int i = 1; args[i] != NULL && index < MAX_PATHS - 1; i++) {
            path[index++] = strdup(args[i]);
        }
        path[index] = NULL;

        // Asegurarse de que el path tenga al menos un valor (por defecto /bin)
        if (path[0] == NULL) {
            path[0] = strdup("/bin");
            path[1] = NULL;
        }
        
        return 0;
    }
    return -1;
}

// Función para ejecutar comandos externos
void execute_command(char **args) {
    if (path[0] == NULL) {
        // Si el path está vacío, no se pueden ejecutar comandos externos
        print_error();
        exit(1);
    }

    for (int i = 0; path[i] != NULL; i++) {
        char command_path[BUFFER_SIZE];
        snprintf(command_path, sizeof(command_path), "%s/%s", path[i], args[0]);
        if (access(command_path, X_OK) == 0) {
            execv(command_path, args);
            // Si execv falla, imprimimos un error y salimos
            print_error();
            exit(1);
        }
    }
    // Si llegamos aquí, el comando no se encontró en ningún directorio del path
    print_error();
    exit(1);
}

// Función para manejar redirección de salida
int handle_redirection(char **args) {
    int i = 0;
    int redirect_index = -1;
    int redirect_count = 0;

    // Verificar la presencia y validez de la redirección
    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            redirect_count++;
            if (redirect_count > 1 || args[i + 1] == NULL || args[i + 2] != NULL) {
                return -1;
            }
            redirect_index = i;
        }
        i++;
    }

    // Si hay redirección, manejarla
    if (redirect_index != -1) {
        if (redirect_index == 0) {
            // No hay comando antes de '>'
            return -1;
        }

        int fd = open(args[redirect_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
            close(fd);
            return -1;
        }
        close(fd);
        args[redirect_index] = NULL;
    }
    return 0;
}

