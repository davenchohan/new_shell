/*
 * Filename: cshell.c
 * Description: Unix Command Interpreter (shell)
 * Date: June 1, 2022
 * Name: Daven Chohan
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

//Struct for the tokenized version of a command
typedef struct {
    char** tokens;
    int token_counter;
} UserCommand;

//Struct for environment variables
typedef struct {
    char *name;
    char *value;
} EnvVar;

//Struct for the linked list of environment variables
typedef struct envl{
    EnvVar* var;
    struct envl* next;
} EnvList;

//Struct for each command in the log
typedef struct {
    char *name;
    struct tm time;
    int value;
} Command;

//Struct for the dynamic array of logs
typedef struct {
    Command **array;
    size_t count;
    size_t capacity;
} Logs;

//Function Declarations

//Functions for environment variables
EnvVar* constructVar(char* name, char* value);
EnvList* construct_env_list(EnvVar* newVar);
void free_envlist(EnvList* list);
EnvVar* search_list(char* varName, EnvList* list);

//Functions for logs
Command* logCommand(char *name, int value);
void insertNewLog(Logs *logs, Command* log);
void printLogs(Logs *logs);
void freeLogs(Logs *logs);

//Functions for the commands
char* read_input(char* input);
void print_command(int token_counter, char** tokens);
int theme(int token_counter, char** tokens);
UserCommand tokenize(char** input);
int execute_command(UserCommand *command);
int choose_command(UserCommand *command, char* original_command, EnvList** variables, Logs* logs);

//Constructor for an environment variable
EnvVar* constructVar(char* name, char* value){
    EnvVar* var = malloc(sizeof(EnvVar));
    var->name = strdup(name);
    var->value = strdup(value);
    return var;
}

//Constructor for environment variable linked list
EnvList* construct_env_list(EnvVar* newVar){
    EnvList* linkedList = malloc(sizeof(EnvList));
    linkedList->var = newVar;
    linkedList->next = NULL;
    return linkedList;
}

//Add to the linked list of environment variables
EnvList* add_env_list(EnvList* oldList, EnvVar* newVar){
    EnvList* linkedList = construct_env_list(newVar);
    linkedList->next = oldList;
    return linkedList;
}

//Free the memory that was allocated for the linked list of environment variables
void free_envlist(EnvList* list){
    EnvList* temp;
    while(list != NULL){
        temp = list;
        list = list->next;
        free(temp->var->name);
        free(temp->var->value);
        free(temp->var);
        free(temp);
    }
}

//Search the linked list of environment variables to see if an entry with the varName exists
EnvVar* search_list(char* varName, EnvList* list){
    EnvList* currentList = list;
    while(currentList != NULL){
        EnvVar* currentVar = currentList->var;
        if (strcmp(currentVar->name, varName) == 0)
        {
            return currentVar; //Entry has been found
        }
        else {
            currentList = currentList->next;
        }
    }
    EnvVar* error = constructVar("error", "error"); //Return an error indicating that varName is not an existing environment variable
    return error;
}

//Constructor for a log entry
Command* logCommand(char *name, int value){
    Command* newCommand = malloc(sizeof(Command));
    time_t rawTime;
    struct tm * currentTime;
    newCommand->name = strdup(name);
    newCommand->value = value;
    time(&rawTime);
    currentTime = localtime(&rawTime);
    newCommand->time = *currentTime;
    return newCommand;
}

//Create and initialize the array of command logs
void initializeLogs(Logs *logs, size_t buffer){
    logs->array = malloc(sizeof(Command) * buffer);
    logs->count = 0;
    logs->capacity = buffer;
}

//Add a new log to the array that holds all command logs
void insertNewLog(Logs *logs, Command* log){
    if (logs->count == logs->capacity)
    {
        logs->capacity = (logs->capacity)*2;
        logs->array = realloc(logs->array, sizeof(Command) * logs->capacity);
    }
    logs->array[logs->count] = log;
    logs->count++;
}

//Print all command logs
void printLogs(Logs *logs){
    for (int i = 0; i < logs->count; ++i)
    {
        Command* currentCommand = logs->array[i];
        char* name = currentCommand->name;
        int value = currentCommand->value;
        char strValue[5];
        sprintf(strValue, "%d", value);
        struct tm* currentTime = &(currentCommand->time);
        write(STDOUT_FILENO, asctime (currentTime), strlen(asctime(currentTime))); //print
        write(STDOUT_FILENO, " ", strlen(" "));
        write(STDOUT_FILENO, name, strlen(name));
        write(STDOUT_FILENO, " ", strlen(" "));
        write(STDOUT_FILENO, strValue, strlen(strValue));
        write(STDOUT_FILENO, "\n", strlen("\n"));
    }
}

//Free the dynamically allocated array of command logs
void freeLogs(Logs *logs){
    for (int i = 0; i < logs->count; ++i)
    {
        Command* currentCommand = logs->array[i];
        char* name = currentCommand->name;
        free(name);
        free(currentCommand);
    }
    free(logs->array);
}

//Read input from the user dynamically
char* read_input(char* input){
    input = NULL;
    char buffer[100];
    int bufferlength = 0;
    int inputlength = 0;
    do {
        fgets(buffer, 100, stdin); // get first 100 characters
        bufferlength = strlen(buffer);
        if (!input)
        {
            input = malloc(bufferlength+1); //malloc memory for first 100 char
        }
        else {
            input = realloc(input, bufferlength+inputlength+1); //realloc memory for next 100 char
        }
        strcpy(input+inputlength, buffer); //copy the chars back to input
        inputlength += bufferlength;
    } while(bufferlength == 99); //while there are still characters left
    return input;
}

//Function for the built-in print command, prints what ever the user enters
void print_command(int token_counter, char** tokens) {
    for (int i = 1; i < token_counter; ++i)
    {
        write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
        if (i != token_counter)
        {
            write(STDOUT_FILENO, " ", strlen(" "));
        }
    }
    write(STDOUT_FILENO, "\n", strlen("\n"));
}

//Function for the built-in theme command
int theme(int token_counter, char** tokens){
    if (token_counter != 2) //Too many words to be the theme red, blue or green
    {
        write(STDOUT_FILENO, "unsupported theme\n", strlen("unsupported theme\n")); //print
        //print_command(token_counter, tokens);
        return -1;
    } else if (strcmp(tokens[1], "red") == 0) //choose the red theme
    {
        write(STDOUT_FILENO, "\033[0;31m", strlen("\033[0;31m"));
        return 0;
    } else if (strcmp(tokens[1], "green") == 0) //choose the green theme
    {
        write(STDOUT_FILENO, "\033[0;32m", strlen("\033[0;32m"));
        return 0;
    } else if (strcmp(tokens[1], "blue") == 0) //choose the blue theme
    {
        write(STDOUT_FILENO, "\033[0;34m", strlen("\033[0;34m"));
        return 0;
    } else {
        write(STDOUT_FILENO, "unsupported theme\n", strlen("unsupported theme\n")); //theme was one that is not supported
        //print_command(token_counter, tokens);
        return -1;
    }
}

//Add a new environment variable or change an existing one
int executeVariableAdd(char* first_word, EnvList** variables){
        char* words[] = {"temp1", "temp2"};
        char* current_word = strdup(first_word);
        char* tok  = strtok(current_word, "=");
        words[0] = tok;
        tok = strtok(NULL, "=");
        words[1] = tok;
        if (!words[1]) //No word was entered after the = sign
        {
            write(STDOUT_FILENO, "Error: No name and/or value found for setting up Environment Variables.\n", strlen("Error: No name and/or value found for setting up Environment Variables.\n")); //print
            free(current_word);
            return -1;
        }
        EnvVar* search = search_list(words[0], (*variables)); //Check if an environment variable with this name already exists
        if (strcmp((*variables)->var->name, "temp") == 0) //If this is the first environment variable being created
        {
            free((*variables)->var->name);
            free((*variables)->var->value);
            free((*variables)->var);
            free(search->name);
            free(search->value);
            free(search);
            (*variables)->var = constructVar(words[0], words[1]);
            free(current_word);
        } else if (strcmp(search->name, "error") != 0) { //If an environment variable with this name already exists, change its entry
            free((*variables)->var->value);
            search->value = strdup(words[1]);
            free(current_word);
        } else { //Add a new environment variable to the linked list
            free(search->name);
            free(search->value);
            free(search);
            EnvVar* addedVar = constructVar(words[0], words[1]);
            *variables = add_env_list(*variables, addedVar);
            free(current_word);
        }
        return 0;
}

//Tokenize the user's input. Seperate each word into its own string in a dynamically allocated array of strings.
UserCommand tokenize(char** input){
    char* current_token = NULL;
    int bufferlength = 5;
    char** tokens = malloc(sizeof(char*) * bufferlength);
    int token_counter = 0;
    char delims[]=" \t\r\n\v\f\a";

    current_token = strtok(*input, delims); //get the first token

    while(current_token != NULL) {
        tokens[token_counter] = current_token; //Add token to the array
        token_counter++;
        if (bufferlength > token_counter)
        {
            bufferlength += 5;
            tokens = realloc(tokens, sizeof(char*) * bufferlength);
        }
        current_token = strtok(NULL, delims); //get the next token
    }
    tokens[token_counter] = NULL;
    UserCommand command = {tokens, token_counter};
    return command;
}

//Function to execute a non-built-in command
int execute_command(UserCommand *command) {
    char** input = command->tokens;
    int pipefd[2]; //Either ends of the pipe
    if (pipe(pipefd) == -1)
    {
        //Error in making the pipe
        write(STDOUT_FILENO, "Missing keyword or command, or permission problem\n", strlen("Missing keyword or command, or permission problem\n")); //print
        exit(EXIT_FAILURE);
    }
    int fc = fork(); //Create a child process
    if (fc < 0)
    {
        //Fork failed
        write(STDOUT_FILENO, "Missing keyword or command, or permission problem\n", strlen("Missing keyword or command, or permission problem\n")); //print
    } else if (fc == 0) {
        // Child
        dup2(pipefd[1], STDOUT_FILENO); //Redirect all outputs to the write end of the pipe
        dup2(pipefd[1], STDERR_FILENO); //Redirect all errors to the write end of the pipe
        close(pipefd[0]); // close the read end
        close(pipefd[1]); //Close the write end
        execvp(input[0], input); //Execute the command
        //Error in the exec or invalid command
        write(STDOUT_FILENO, "Missing keyword or command, or permission problem\n", strlen("Missing keyword or command, or permission problem\n")); //print
        exit(EXIT_FAILURE);
    } else {
        // Parent
        wait(NULL); //Wait for child process to finish
        close(pipefd[1]); //Close the write end of the pipe
        //Dynamically read the read end of the pipe
        char* output = NULL;
        char buffer[100];
        memset(buffer, 0, 100);
        int bufferlength = 0;
        int outputlength = 0;
        do {
            read(pipefd[0], buffer, sizeof(buffer)); // get first 100 characters
            bufferlength = strlen(buffer);
            if (!output)
            {
                output = malloc(bufferlength+1); //malloc memory for first 100 char
            }
            else {
                output = realloc(output, bufferlength+outputlength+1); //realloc memory for next char
            }
            strcpy(output+outputlength, buffer); //copy the chars back to input
            outputlength += bufferlength;
        } while(bufferlength == 99); //while there are still characters left
        write(STDOUT_FILENO, output, strlen(output)); //print
        free(output);
        close(pipefd[0]);
    }
    return 1;
}

//Function to determine what command the user has entered
int choose_command(UserCommand *command, char* original_command, EnvList** variables, Logs* logs){
    char** tokens = command->tokens;
    int token_counter = command->token_counter;
    //Replace all environment variables with its corresponding value
    if (token_counter >1)
    {
        for (int i = 0; i < token_counter; ++i)
        {
            char* current_word = tokens[i];
            EnvVar* search = search_list(current_word, *variables); //Check to see if the environment variable exists
            if (strcmp(search->name, "error") != 0)
            {
                strcpy(tokens[i], search->value); //If it exists replace its name with its value
                break;
            }
            free(search->name);
            free(search->value);
            free(search);
        }
    }

    char* first_word = tokens[0];
    if (!first_word) //If the user inputs empty space
    {
        write(STDOUT_FILENO, "Missing keyword or command, or permission problem\n", strlen("Missing keyword or command, or permission problem\n")); //print
        return -1;
    }
    else if (strcmp(first_word, "print") == 0) //If the user chooses the built-in print command
    {
        print_command(token_counter, tokens);
        return 0;
    } else if (strcmp(first_word, "theme") == 0) //If the user chooses the built-in theme command
    {
        int themeStatus = theme(token_counter, tokens); 
        return themeStatus;
    } else if (strcmp(first_word, "exit") == 0) //if command is exit, leave the shell and free all allocated memory
    {
        free(tokens);
        freeLogs(logs);
        free(original_command);
        free_envlist(*variables);
        write(STDOUT_FILENO, "Bye!\n", strlen("Bye!\n"));
        exit(EXIT_SUCCESS);
    } else if (first_word[0] == '$') //If the first character is $ it means they are attempting to create an environment variable
    {
        if (token_counter == 1)
        {
            return executeVariableAdd(first_word, variables);
        } else { //If the user did not follow the correct format or missesd the name or value
            write(STDOUT_FILENO, "Error: No name and/or value found for setting up Environment Variables.\n", strlen("Error: No name and/or value found for setting up Environment Variables.\n")); //print
            return -1;
        }
    } else if (strcmp(first_word, "log") == 0) //If the user chooses the built-in log command
    {
        printLogs(logs);
        return 0;
    }
    else { //Else the user inputted a non-built-in command
        execute_command(command);
        return 0;
    }
}

//Main function to run the program
void main(int argc, char *argv[]){
    char* command; //Used to read the user input if given
    char* currentLine = NULL; //Used to read the file if given
    size_t lineLength = 0;
    ssize_t readLine;
    EnvVar* newVariable = constructVar("temp", "temp"); //Initialize a temporary environment variable
    EnvList* variables = construct_env_list(newVariable); //Initialize the linked list of environment variables
    Logs logs; //Create the array of logs
    initializeLogs(&logs, 5); //Initialize the array of logs
    if (argc == 2) //If the user wants to use script mode
    {
        FILE *file;
        file = fopen(argv[1], "r");
        if (!file) //Error in reading the file
        {
            write(STDOUT_FILENO, "Unable to read script file: ", strlen("Unable to read script file: "));
            write(STDOUT_FILENO, argv[1], strlen(argv[1]));
            exit(EXIT_FAILURE);
        }
        while((readLine = getline(&currentLine, &lineLength, file)) != -1) { //Read the file line by line
            //printf("%s", currentLine);
            UserCommand tokenized_command = tokenize(&currentLine); //Tokenize the input
            int commandStatus = choose_command(&tokenized_command, currentLine, &variables, &logs); //Execute its command
            char** tokens = tokenized_command.tokens;
            Command* log = logCommand((tokens[0]), commandStatus); //Log the command
            insertNewLog(&logs,log); //Insert log into array
            free(tokens); //Free the tokenized input
        }
        //Free everything and exit once the file has been read
        free(currentLine);
        fclose(file);
        freeLogs(&logs);
        free_envlist(variables);
        write(STDOUT_FILENO, "Bye!\n", strlen("Bye!\n")); //print
        exit(EXIT_SUCCESS);
    } 
    else if (argc > 2) //If more than one argument is given
    {
        write(STDOUT_FILENO, "Error: Too many arguments\n", strlen("Error: Too many arguments\n")); //print
    }
    else //Use the shell in interactive mode
    {
        while(1) {
            write(STDOUT_FILENO, "cshell$ ", strlen("cshell$ ")); //print
            command = read_input(command); //read the user input
            UserCommand tokenized_command = tokenize(&command); //Tokenize the input
            int commandStatus = choose_command(&tokenized_command, command, &variables, &logs); //Execute its command
            char** tokens = tokenized_command.tokens;
            Command* log = logCommand((tokens[0]), commandStatus); //Log the command
            insertNewLog(&logs,log); //Insert log into array
            free(tokens); //Free the tokenized input
            free(command); //Free the users initial input
        }
    }
}


