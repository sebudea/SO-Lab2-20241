// Sebastian Aristizabal  y Alejandro Arias

// Librerias
#include <stdio.h>    
#include <stdlib.h>   
#include <string.h>  
#include <unistd.h>  
#include <sys/wait.h> 
#include <fcntl.h>    

#define MAX_PATHS 10 // Máximo número de rutas
#define MAX_ARGS 10  // Máximo número de argumentos
#define ERROR_MESSAGE "An error has occurred\n"

// Arreglo de rutas donde buscar los ejecutables de comandos
char *paths[MAX_PATHS] = {"/bin", NULL};

// Función para imprimir un mensaje de error en la salida de errores estándar
void printError()
{
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}

// Función que ejecuta el comando dado con posibles redirecciones
void executeCommand(char **args, char *outputFile, int redirect)
{
    if (!args[0])
        return; // Si no hay un comando, salir de la función

    // Comando interno: "exit"
    if (strcmp(args[0], "exit") == 0)
    {
        if (args[1]) // Si "exit" tiene argumentos adicionales, muestra error
        {
            printError();
        }
        else
        {
            exit(0); // Salir del programa
        }
    }
    // Comando interno: "cd" (cambiar directorio)
    else if (strcmp(args[0], "cd") == 0)
    {
        if (!args[1] || args[2]) // Si "cd" no tiene o tiene más de un argumento, muestra error
        {
            printError();
        }
        else
        {
            if (chdir(args[1]) != 0) // Cambia al directorio especificado y muestra error si falla
                printError();
        }
    }
    // Comando interno: "path" (establecer rutas de búsqueda)
    else if (strcmp(args[0], "path") == 0)
    {
        memset(paths, 0, sizeof(paths)); // Limpia el arreglo paths
        for (int i = 1; args[i] && i < MAX_PATHS; i++)
        {
            paths[i - 1] = strdup(args[i]); // Copia cada ruta dada a paths
        }
    }
    else
    {
        // Comando regular: crea un proceso hijo
        pid_t pid = fork();
        if (pid == 0) // Código del proceso hijo
        {
            // Si se necesita redirección de salida
            if (redirect)
            {
                int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                if (fd < 0) // Error al abrir archivo de salida
                {
                    printError();
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO); // Redirige stdout al archivo
                dup2(fd, STDERR_FILENO); // Redirige stderr al archivo
                close(fd);
            }

            // Intenta ejecutar el comando en cada ruta disponible
            char execPath[100];
            for (int i = 0; paths[i]; i++)
            {
                snprintf(execPath, sizeof(execPath), "%s/%s", paths[i], args[0]);
                if (access(execPath, X_OK) == 0) // Verifica si el comando es ejecutable
                {
                    execv(execPath, args); // Ejecuta el comando
                }
            }
            printError(); // Muestra error si falla en todas las rutas
            exit(1);
        }
        else if (pid < 0) // Error en la creación del proceso
        {
            printError();
        }
        // No espera al proceso hijo aquí; el proceso padre continúa
    }
}

// Función para analizar la línea de comandos y ejecutar cada comando
void parseAndExecute(char *line)
{
    char *command, *saveptr, *args[MAX_ARGS], *outputFile = NULL;
    int redirect = 0;

    // Divide los comandos separados por "&"
    command = strtok_r(line, "&", &saveptr);

    while (command)
    {
        char *token = strtok(command, " \t\n"); // Obtiene el primer token de comando
        int i = 0;

        if (token && strcmp(token, ">") == 0) // Error si el primer token es ">"
        {
            printError();
            return;
        }

        // Análisis del comando y redirección
        while (token && i < MAX_ARGS)
        {
            if (strcmp(token, ">") == 0) // Detecta redirección de salida
            {
                redirect = 1;                  // Activa redirección
                token = strtok(NULL, " \t\n"); // Siguiente token debe ser el archivo de salida
                if (!token || strtok(NULL, " \t\n")) // Error si no hay archivo o hay demasiados argumentos
                {
                    printError();
                    return;
                }
                outputFile = token;
                break;
            }
            args[i++] = token;             // Almacena el token en los argumentos
            token = strtok(NULL, " \t\n"); // Obtiene el siguiente token
        }
        args[i] = NULL; // Termina la lista de argumentos con NULL

        // Ejecuta el comando en el proceso actual
        executeCommand(args, outputFile, redirect);

        command = strtok_r(NULL, "&", &saveptr); // Pasa al siguiente comando
    }

    // Espera a que todos los procesos hijos terminen
    while (wait(NULL) > 0)
        ;
}

// Función principal que inicia el intérprete de comandos
int main(int argc, char *argv[])
{
    FILE *input = stdin; 
    if (argc == 2) // Si se proporciona un archivo de entrada como argumento
    {
        input = fopen(argv[1], "r"); // Abre el archivo para lectura
        if (input == NULL) // Error al abrir el archivo
        {
            printError();
            exit(1);
        }
    }
    else if (argc > 2) // Error si hay más de un argumento
    {
        printError();
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    while (1)
    {
        if (argc == 1) // Muestra el prompt solo en modo interactivo
        {
            printf("wish> ");
        }
        if (getline(&line, &len, input) == -1) // Lee la línea de entrada
        {
            break; // Termina si llega al final del archivo (EOF)
        }
        parseAndExecute(line); // Analiza y ejecuta la línea de comando
    }

    free(line);       // Libera la memoria de la línea leída
    if (argc == 2)    // Cierra el archivo de entrada si fue abierto
    {
        fclose(input);
    }
    return 0;
}
