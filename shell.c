// compilation: gcc -o sh shell.c hashmap.c
// Interactive/REPL Mode (default): ./sh
// Batch/Scripting Mode: ./sh script

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include "hashmap.h" // homemade hashmap ;)

#define CMD_BUFFER_LEN 2000
#define ERR_BUFFER_LEN 256
#define CWD_BUFFER_LEN 200
#define MAX_ARGS 100

// extensible command database
#define NUM_CMDS 8
#define HM_SIZE 17 // prime approx 2 * NUM_CMDS
char *builtInCommands[NUM_CMDS] = {"exit", "cd", "pwd", "ls", "cat", "touch", "mkdir", "rm"};
enum commandIndices               { EXIT,   CD,   PWD,   LS,   CAT,   TOUCH,   MKDIR,   RM};
HashMap *cmd_to_index;

// wrappers
void construct_cmd_database();
void execute(int arg_count, char ** args);
void run_cmd(char *command_buffer);
void process_job(char *job);

// commands
void shell_cd(int arg_count, char **args);
void shell_exit(int arg_count, char **args);
void shell_pwd(int arg_count, char **args);
void shell_ls(int arg_count, char **args);
void shell_cat(int arg_count, char **args);
void shell_touch(int arg_count, char **args);
void shell_mkdir(int arg_count, char **args);
void shell_rm(int arg_count, char **args);

// main: Separates Interactive and Batch Mode
int main(int argc, char *argv[]) {
    construct_cmd_database();

    // Batch Mode
    if (argc == 2) {
        char *filename = argv[1];
        FILE *file = fopen(filename, "r");
        if (file == NULL) {
            perror("Error opening script");
            exit(EXIT_FAILURE);
        }

        char cmd_buff[CMD_BUFFER_LEN];
        while (fgets(cmd_buff, CMD_BUFFER_LEN, file) != NULL) {
            run_cmd(cmd_buff);

            fclose(file);
            exit(EXIT_SUCCESS);
        }
    }
    // Interactive Mode
    char cmd_buff[CMD_BUFFER_LEN];
    char *pinput;
    while (1) {
        printf("shell>"); // prompt
        fflush(stdout);

        if (fgets(cmd_buff, CMD_BUFFER_LEN, stdin) == NULL) {
            printf("error: pre-emptive EOF detected\n");
            exit(0);
        }
        int input_too_long = 1;
        for (int i = 0; i < CMD_BUFFER_LEN; i++) {
            if (cmd_buff[i] == '\0') {
                input_too_long = 0;
                break;
            }
        }
        if (input_too_long) { printf("input too long \n"); } 
        else { run_cmd(cmd_buff); }
    }
}

void construct_cmd_database() {
    cmd_to_index = malloc(sizeof(HashMap));
    initHashMap(cmd_to_index, HM_SIZE);
    for (int i = 0; i < NUM_CMDS; i++) {
        hashMapPut(cmd_to_index, builtInCommands[i], i);
    }
}

// run_cmd: wrapper for tokenizing and executing multiple jobs
// use strtok_r to avoid using strtok twice
void run_cmd(char *cmd_buff) {
    const char job_delim[] = ";\t\n\r\v";
    char *job, *saveptr;
    char *cmd_buff_copy = strdup(cmd_buff);

    for (job = strtok_r(cmd_buff_copy, job_delim, &saveptr); 
            job != NULL; 
            job = strtok_r(NULL, job_delim, &saveptr)) {
        char *job_copy = strdup(job);
        process_job(job_copy);
        free(job_copy);
    }
    free(cmd_buff_copy);
}

// process_job: tokenizes and executes a single job
void process_job(char *job) {
    int argc = 0;
    char **args = malloc(MAX_ARGS * sizeof(char*));
    const char delims[] = " ";
    char *arg = strtok(job, delims);
    int redirect = 0, append = 0;
    char *filename = NULL;
    while (arg != NULL) {
        // printf("%s ", arg);
        if (strcmp(arg, ">") == 0 || strcmp(arg, "+>") == 0) {
            append = (arg[0] == '+');
            redirect = 1;
            arg = strtok(NULL, delims);
            filename = arg;
            break; // stop processing further arguments
        }
        args[argc++] = arg;
        arg = strtok(NULL, delims);
    }
    if (redirect && filename != NULL) {
        int flags = append ? (O_RDWR | O_CREAT | O_APPEND) : (O_RDWR | O_CREAT);
        int fd = open(filename, flags, 0644);
        if (fd == -1) {
            perror("open failed");
            free(args);
            return;
        }
        int stdout_copy = dup(STDOUT_FILENO);

        // redirect standard output to specified file
        dup2(fd, STDOUT_FILENO);
        close(fd);
        
        // execute as usual
        // execute is oblivious to the redirection handled by run_cmd
        execute(argc, args);

        // restore standard output for next iteration
        dup2(stdout_copy, STDOUT_FILENO);
        close(stdout_copy);
    } else { execute(argc, args); }
    // printf("\ntotal arguments: %d \n", argc);
    free(args);
}

void execute(int argc, char** args) {
    if (argc == 0) { return; }
    int cmd_index = hashMapGet(cmd_to_index, args[0]);
    if (cmd_index == -1) {
        printf("command %s not found\n", args[0]);
        return;
    }
    switch (cmd_index) {
        case EXIT:
            shell_exit(argc, args);
            break;
        case PWD:
            shell_pwd(argc, args);
            break;
        case CD:
            shell_cd(argc, args);
            break;
        case LS:
            shell_ls(argc, args);
            break;
        case CAT:
            shell_cat(argc, args);
            break;
        case TOUCH:
            shell_touch(argc, args);
            break;
        case MKDIR:
            shell_mkdir(argc, args);
            break;
        case RM:
            shell_rm(argc, args);
            break;
    }
}

void shell_cd(int argc, char **args) {
    if (argc == 1) { chdir(getenv("HOME")); } 
    else if (argc == 2) {
        if (chdir(args[1]) != 0) {
            printf("%s not a valid directory\n", args[1]);
        }
    } else { printf("change directory failed\n"); }
}

void shell_exit(int argc, char **args) {
    if (argc == 1) { exit(0); } 
    else { printf("exit must be used by itself\n"); }
}

void shell_pwd(int argc, char **args) {
    if (argc != 1) {
        printf("pwd must be used by itself\n");
        return;
    }
    char cwd_buff[CWD_BUFFER_LEN];
    if (getcwd(cwd_buff, CWD_BUFFER_LEN) != NULL) {
        printf("%s\n", cwd_buff);
    } else { printf("current working directory not found\n"); }
}

// not a syscall, using the external "ls" program
// need to fork a child process to avoid termination of shell
void shell_ls(int argc, char **args) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("system error: fork failed in ls");
    } else if (pid == 0) { // child process
        execvp("ls", args);
        // this code only runs if execvp fails
        perror("system error: execvp failed in ls");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0); // wait for child process to finish
    }
}

void shell_cat(int argc, char **args) {
    if (argc < 2) {
        printf("cat: file name not specified\n");
        return;
    }
    // open all file arguments after "cat"
    for (int i = 1; i < argc; i++) {
        int fd = open(args[i], O_RDONLY);
        if (fd == -1) {
            perror("cat open failed");
            continue;
        }
        char buffer[1024];
        ssize_t bytes_read;
        // while loop in case buffer needs to make several trips
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        close(fd); // close and flush
    }
}

void shell_touch(int argc, char **args) {
    if (argc < 2) {
        printf("touch: file name not specified\n");
        return;
    }
    // touch all file arguments after "touch"
    for (int i = 1; i < argc; i++) {
        int fd = open(args[i], O_WRONLY | O_CREAT, 0644);
        if (fd == -1) { perror("touch open failed"); }
        else { close(fd); }
    }
}

void shell_mkdir(int argc, char **args) {
    if (argc < 2) {
        printf("mkdir: directory name not specified\n");
        return;
    }
    // make all directory arguments after "mkdir"
    for (int i = 1; i < argc; i++) {
        if (mkdir(args[i], 0755) == -1) {perror("mkdir failed"); }
    }
}

void rm_dir_r(const char *dirname) {
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        perror("Failed to open directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            // skip current and parent to avoid infinite recursion and
            // modifying the parent
            continue;
        }

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        struct stat path_stat;
        stat(path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) { rm_dir_r(path);}
        else {unlink(path);}
    }
    closedir(dir);
    rmdir(dirname);
}

void shell_rm(int argc, char **args) {
    if (argc < 2) {
        printf("rm: missing operand\n");
        return;
    }

    int recursive = 0;
    int arg_start = 1;
    if (strcmp(args[1], "-r") == 0) {
        recursive = 1;
        arg_start = 2;
    }

    for (int i = arg_start; i < argc; i++) {
        struct stat path_stat;
        stat(args[i], &path_stat);
        if (recursive && S_ISDIR(path_stat.st_mode)) {
            rm_dir_r(args[i]);
        } else {
            if (unlink(args[i]) == -1) {
                perror("rm failed");
            }
        }
    }
}

