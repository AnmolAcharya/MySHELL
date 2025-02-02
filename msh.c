
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define WHITESPACE " \t\n"
#define MAX_COMMAND_SIZE 255    
#define MAX_NUM_ARGUMENTS 32    

// Function declarations
void error_handling();
void execute_batch_mode(FILE *user_input);
void execute_builtin_command(char *command_string);
void execute_command(char *tokens[]);
void handle_cd(char *tokens[]);

int main(int argc, char *argv[]) 
{
    FILE *user_input = stdin;

    if (argc > 1)  // Batch mode implementation
    {
        if (argc > 2) 
        {
            error_handling();  // Handle cases when we have more files
            exit(EXIT_FAILURE);
        }
        
        user_input = fopen(argv[1], "r");  // Opens the file for reading
        if (user_input == NULL) 
        {
            error_handling();  // Error handling for fopen()
            exit(EXIT_FAILURE);
        }
        
        execute_batch_mode(user_input);
    } 
    else 
    {
        while (1)   // Interactive Mode!
        {  
            char *command_string = (char *)malloc(MAX_COMMAND_SIZE);
            printf("msh> ");  // Prints prompt only in interactive mode in the while loop

            if (!fgets(command_string, MAX_COMMAND_SIZE, user_input))  // Read input from stdin or file
            { 
                break;
            }

            execute_builtin_command(command_string);  // Call the built-in command handler
            free(command_string);
        }
    }

    if (user_input != stdin) 
    { 
        fclose(user_input);  // Close file
    }

    return 0;
}

void error_handling()   // Error function to print error where needed 
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void execute_batch_mode(FILE *user_input)  // Handles batch mode commands
{
    char *command_string = (char *)malloc(MAX_COMMAND_SIZE); 
    while (fgets(command_string, MAX_COMMAND_SIZE, user_input)) 
    {
        execute_builtin_command(command_string);  // Handles batch mode errors (testcase 1) 
    }
    free(command_string);
}

void execute_builtin_command(char *command_string)  // Handles built-in commands: exit, quit, cd...
{
    char *tokens[MAX_NUM_ARGUMENTS];
    int token_count = 0;
    char *argument_pointer;  // Stores individual tokens
    char *working_string = strdup(command_string);  // Makes a duplicate of the command string 
    char *head_ptr = working_string;  // Save the pointer to free later
    
    while ((argument_pointer = strsep(&working_string, WHITESPACE)) != NULL && token_count < MAX_NUM_ARGUMENTS) 
    {
        if (strlen(argument_pointer) == 0) 
        {
            continue;  // Skip whitespaces if present (*test case 15)
        }
        
        tokens[token_count] = strndup(argument_pointer, MAX_COMMAND_SIZE);
        token_count++;
    }
    tokens[token_count] = NULL;  // Null-terminate the tokens array to work with execvp (testcase 15)

    if (tokens[0] == NULL)  // If no command is provided, skip it
    {
        free(head_ptr);
        return;
    }

    if (strcmp(tokens[0], "exit") == 0 || strcmp(tokens[0], "quit") == 0)  // Built-in commands: exit, quit
    {
        if (token_count > 1)  // Throw error if more than 1 argument (*not 2 as it interferes with 5.err)
        {
            error_handling();
        } 
        free(head_ptr); 
        exit(0);
    } 
    else if (strcmp(tokens[0], "cd") == 0) 
    {
        handle_cd(tokens);
        free(head_ptr);
        return;
    }

    execute_command(tokens);

    free(head_ptr);  // Free allocated memory
    for (int i = 0; i < token_count; i++) 
    {
        free(tokens[i]);
    }
}

void handle_cd(char *tokens[])  // Handle the cd command
{
    if (tokens[1] == NULL) 
    {
        error_handling();
    }
    else 
    {
        if (chdir(tokens[1]) != 0) 
        {
            error_handling();  // Testcase 2
        }
    }
}

void execute_command(char *tokens[])  // Fork & execvp implementation
{
    if (tokens[0] == NULL || strcmp(tokens[0], "") == 0)  // No command provided
    {
        error_handling();
    } 

    pid_t pid = fork();
    if (pid == -1)  // When fork() returns -1, an error occurred
    {
        error_handling();  // Testcase 5
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0)  // Child process
    {
        for (int i = 1; tokens[i] != NULL; i++) 
        {
            if (strcmp(tokens[i], ">") == 0)  // Output redirection
            { 
                if (tokens[i + 2] != NULL)  // Argument check (*testcase 8)
                { 
                    error_handling(); 
                    exit(EXIT_FAILURE); 
                }

                int fd = open(tokens[i + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                if (fd < 0) 
                {
                    error_handling();  // Error handling for open()
                    exit(EXIT_FAILURE);
                }
                
                dup2(fd, STDOUT_FILENO); 
                close(fd);
                tokens[i] = NULL;
                break;
            }
        }

        execvp(tokens[0], tokens);  // Execute the command
        error_handling(); 
        exit(EXIT_FAILURE);
    } 
    else  // Parent process
    { 
        int status;  
        waitpid(pid, &status, 0);  // Wait for the child to finish
    }
}
