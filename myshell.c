#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // exit()
#include <unistd.h>   // fork(), getpid(), exec()
#include <sys/wait.h> // wait()
#include <signal.h>   // signal()
#include <fcntl.h>    // close(), open()
#include <limits.h>   // PATH_MAX

void executeCommand(char **execCmd)
{
    // This function will fork a new process to execute a command
    if (strcmp(execCmd[0], "cd") == 0) // if cd is there change dir first
    {
        chdir(execCmd[1]);
    }
    else
    {
        int pid = fork();
        if (pid < 0)
        {
            printf("Fork Error\n");
        }
        else if (pid == 0) // child process
        {
            signal(SIGTSTP, SIG_DFL); //  singla handling
            signal(SIGINT, SIG_DFL);
            if (execvp(execCmd[0], execCmd) == -1)
            {
                printf("Shell: Incorrect command\n");
            }
            exit(0);
        }
        else
        {
            wait(NULL);
        }
    }
}

void Redirect(char **execCmd)
{
    int pid = fork();
    if (pid < 0)
    {
        printf("Fork error");
    }
    else if (pid == 0)
    {
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int rs = open(execCmd[2], O_CREAT | O_TRUNC | O_WRONLY, mode);
        close(STDOUT_FILENO);
        dup2(rs, 1);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        if (execvp(execCmd[0], execCmd) == -1)
        {
            printf("Shell: Incorrect command\n");
        }
        exit(0);
    }
    else
    {
        wait(NULL);
    }
}

void executeCommandParallel(char **execCmd)
{
    // This function will fork a new process to execute a command
    if (strcmp(execCmd[0], "cd") == 0)
    {
        chdir(execCmd[1]);
    }
    else // no wait for child process
    {
        int pid = fork();
        if (pid < 0)
        {
            printf("Fork Error\n");
        }
        else if (pid == 0)
        {
            signal(SIGTSTP, SIG_DFL);
            signal(SIGINT, SIG_DFL);
            if (execvp(execCmd[0], execCmd) == -1)
            {
                printf("Shell: Incorrect command\n");
            }
            exit(0);
        }
    }
}

void executeParallelCommands(size_t bufsize, char **tokens)
{
    int i = 0;
    // i for tokens and j for execCmd
    while (tokens[i] != NULL)
    {
        char **execCmd = malloc(bufsize * sizeof(char *));
        int j = 0;
        while (tokens[i] != NULL && strcmp(tokens[i], "&&") != 0) // remove &&
        {
            execCmd[j] = tokens[i];
            i++;
            j++;
        }
        if (tokens[i] != NULL)
        {
            i++;
        }
        execCmd[j] = NULL;
        // execute the command till &&
        executeCommandParallel(execCmd);
        free(execCmd);
    }
    wait(NULL);
}

void executeSequentialCommands(size_t bufsize, char **tokens)
{
    int i = 0;

    // i for tokens and j for execCmd

    while (tokens[i] != NULL)
    {
        char **execCmd = malloc(bufsize * sizeof(char *));
        int j = 0;
        while (tokens[i] != NULL && strcmp(tokens[i], "##") != 0) // remove ##
        {
            execCmd[j] = tokens[i];
            i++;
            j++;
        }
        if (tokens[i] != NULL)
        {
            execCmd[j] = NULL;
            // execute the command till ##
            executeCommand(execCmd);
            i++;
        }
        else
        {
            execCmd[j] = NULL;
            // execute the command till ##
            executeCommand(execCmd);
        }
        free(execCmd);
    }
}

void executeCommandRedirection(size_t bufsize, char **tokens)
{
    char **execCmd = malloc(bufsize * sizeof(char *));
    if (tokens[0] != NULL) 
    {
        execCmd[0] = tokens[0]; 
        execCmd[1] = NULL;
    }
    execCmd[2] = tokens[2];
    Redirect(execCmd);
}

char **parseInput(char *buffer, size_t bufsize)
{
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token = strtok(buffer, " \n");
    int position = 0;
    while (token != NULL) // parse newline and spaces
    {
        tokens[position] = token;
        position++;
        token = strtok(NULL, " \n");
    }
    tokens[position] = NULL;
    return tokens;
}

int Check(char *buffer, int *len)
{
    int status; // 0 if single command 1 if it contain ##    2 if contain &&  3 if contain >
    int i = 0;
    while (buffer[i] != '\n')
    {
        if (buffer[i] == '#')
        {
            status = 1;
            break;
        }
        if (buffer[i] == '&')
        {
            status = 2;
            (*len)++;
        }
        if (buffer[i] == '>')
        {
            status = 3;
            break;
        }
        i++;
    }
    return status;
}

int main()
{
    // Initial declarations
    char *buffer;
    size_t bufsize = 64;
    char cwd[PATH_MAX];
    int status;
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    while (1)
    {
        // Current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s$", cwd);
        }

        //accept the input with getline()
        buffer = (char *)malloc(bufsize * sizeof(char));
        getline(&buffer, &bufsize, stdin);

        if (strcmp(buffer, "exit\n") == 0)
        {
            printf("Exiting shell...\n");
            break;
        }

        // now we will check which type of command it is
        int len = 0;
        status = Check(buffer, &len);

        // execute the line as per need
        if (status == 1) // sequential command ##
        {
            char **tokens;
            // tokens = array of char * to commands with spaces
            tokens = parseInput(buffer, bufsize);

            // Call Sequential Command Function
            executeSequentialCommands(bufsize, tokens);
        }
        else if (status == 2) // Parallel command &&
        {
            char **tokens;
            // tokens = array of char * to commands with spaces
            tokens = parseInput(buffer, bufsize);

            // Call Sequential Command Function
            len /= 2; // no of and's
            len++;    // no of cmd
            executeParallelCommands(bufsize, tokens);
            wait(NULL);
        }
        else if (status == 3) // Output redirection >
        {
            char **tokens;
            // parse by spaces
            tokens = parseInput(buffer, bufsize);
            // Call the execute command function
            executeCommandRedirection(bufsize, tokens);
        }
        else // Single Command
        {
            char **tokens;
            // parse by spaces
            tokens = parseInput(buffer, bufsize);
            // Call the execute command function
            executeCommand(tokens);
        }
    }
    return 0;
}
