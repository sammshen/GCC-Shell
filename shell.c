#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_LEN 2000
#define JOB_LEN 50
#define ARG_LEN 100

#define CWD_LEN 200

#define BUILT_IN_LEN 3
char *builtInCommands[BUILT_IN_LEN] = {"exit", "cd", "pwd"};

char error_message[30] = "An error has occurred\n";

void my_print(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void my_exit() {
    // my_print("my_exit: ");
    exit(0);
}

void my_cd(int argc, char **args) {
    if (argc == 1) {
        chdir(getenv("HOME"));
    } else if (argc == 2) {
        if (chdir(args[1]) != 0) {
            my_print(error_message);
        }
    } else {
        my_print(error_message);
        return;
    }
}

void my_pwd() {
    char cwd_buff[CWD_LEN];
    if (getcwd(cwd_buff, sizeof(cwd_buff)) != NULL) {
        my_print(cwd_buff);
        my_print("\n");
    } else {
    //    my_print("in my_pwd: ");
        my_print(error_message);
    }
}

char **get_jobs(char *cmd_buff) {
    char **jobs = malloc(JOB_LEN * sizeof(char*));

    char *delims = ";";
    char *j = strtok(cmd_buff, delims);

    int i = 0;
    while (j != NULL) {
        jobs[i] = j;
        j = strtok(NULL, delims);
        i++;
    }
    jobs[i] = NULL;
    return jobs;
}

char **get_args(char *job_buff, int *argc) {
    char **args = malloc(ARG_LEN * sizeof(char*));

    char *delims = "\t\n\r\v ";
    char *arg = strtok(job_buff, delims);

    int arg_cnt = 0;
    while (arg != NULL) {
        char buff[514];
        int arg_pos = 0;
        int buff_pos = 0;
        while (arg[arg_pos] != '\0') {
            if (arg[arg_pos] == '>') {
                if (arg_pos != 0) {
                    buff[buff_pos] = '\0';
                    args[arg_cnt++] = strdup(buff);
                    buff_pos = 0;
                }
                if (arg[arg_pos + 1] == '+') {
                    arg_pos++;
                    args[arg_cnt++] = ">+";
                } else {
                    args[arg_cnt++] = ">";
                }
            } else {
                buff[buff_pos++] = arg[arg_pos];
                if (arg[arg_pos + 1] == '\0') {
                    buff[buff_pos] = '\0';
                    args[arg_cnt++] = strdup(buff);
                }
            }
            arg_pos++;
        }
        arg = strtok(NULL, delims);
    }
    args[arg_cnt] = NULL;
    *argc = arg_cnt;
    return args;
}

int redir_valid(int argc, char **args) {
    int redir_status = 0;

    char *redir_file = args[argc - 1];
    if (redir_file[0] == '>') {
        // my_print("last arg is redir - bad\n");
        // my_print(error_message);
        return -1;
    }

    for (int i = 0; i < argc - 1; i++) {
        char *arg = args[i];
        if (arg[0] == '>') {
            // my_print("redirection spotted\n");
            if (i != argc - 2) {
                return -1;
            }
            //check file's directory exists
            if (opendir(dirname(redir_file)) == NULL) {
                // my_print("redir file not found\n");
                return -1;
            }
            if (arg[1] == '+') {
                //advanced redirection - do nothing
                redir_status = 2;
                // my_print("Set 2\n");
            } else {
                // my_print("regular redirection spotted\n");
                //regular redirection - ensure that file does not already exist
                FILE *check = fopen(redir_file, "r");
                if (check != NULL) {
                    fclose(check);
                    return -1;
                }
                redir_status = 1;
                // my_print("regular redirection passed\n");
            }
            args[i] = NULL;
        }
    }
    return redir_status;
}

void execute(int argc, char **args) {
    if (argc == 0) {
        return;
    }
    for (int i = 0; i < BUILT_IN_LEN; i++) {
        if (strcmp(args[0], builtInCommands[i]) == 0) {
            if (i == 0) {
                if (argc != 1) {
                    my_print(error_message);
                    return;
                }
                my_exit();
            } else if (i == 1) {
                my_cd(argc, args);
            } else if (i == 2) {
                if (argc != 1) {
                    my_print(error_message);
                    return;
                }
                my_pwd();
            }
            return;
        }
    }

    // -1 is error
    // 0 is no redirection
    // 1 is regular redirection
    // 2 is advanced redirection
    int redir_status = redir_valid(argc, args);
    // printf("redir_status: %d\n", redir_status);
    
    //invalid redirection
    if (redir_status == -1) {
        // my_print("redir error \n");
        my_print(error_message);
        return;
    }
    char *redir_file;
    int NEW_FILENO;
    char buff[6000];

    //check if there is redirection
    if (redir_status != 0) {
        // change stdout for regular and advanced
        redir_file = args[argc - 1];
        // my_print("file is: ");
        // my_print(redir_file);
        // my_print("\n");
        FILE *file = fopen(redir_file, "r");
        if (!file) {
            redir_status = 1;
            file = fopen(redir_file, "w");
        }
        NEW_FILENO = fileno(file);
        //copy file into buffer for advanced
        if (redir_status == 2) {
            
            fread(buff, 1, 6000, file);
            fclose(file);
            // my_print("checkpoint 1\n");
            // my_print("buff is: ");
            // my_print(buff);
            // my_print("buff ended");
            // my_print("\n");
            file = fopen(redir_file, "w");
            NEW_FILENO = fileno(file);
        }
    }
    pid_t pid = fork();

    if (pid == 0) {
        // printf("%s\n", args[1]);
        if (redir_status != 0) {
            // my_print("dup 2 happening\n");
            dup2(NEW_FILENO, STDOUT_FILENO);
        }
        execvp(args[0], args);
        // my_print("after execvp");
        my_print(error_message);
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, WUNTRACED);
        // put buffer back into file for advanced
        if (redir_status != 0) {
            // my_print("checkpoint 2\n");
            if (redir_status == 2 && buff != NULL && strlen(buff) != 0) {
                // my_print("buff: ");
                // my_print(buff);
                // my_print("checkpoint 3\n");
                FILE *file = fopen(redir_file, "a");
                // my_print("checkpoint 4\n");
                // my_print(redir_file);
                // if (file == NULL) {
                //     // my_print("?????\n");
                // }
                fputs(buff, file);
                // my_print("checkpoint 5\n");
                fclose(file);
                // my_print("checkpoint 6\n");
            }
        }
    }
}

void run_cmd(char *cmd_buff) {
    char **jobs = get_jobs(cmd_buff);

    int job_cnt = 0;
    while (jobs[job_cnt] != NULL) {
//        my_print(jobs[job_cnt]);
        int *argc = malloc(sizeof(int));
        *argc = 0;
        char **args = get_args(jobs[job_cnt], argc);
        // my_print("arguments: \n");
        // for (int i = 0; i < *argc; i++) {
        //     my_print(args[i]);
        //     my_print("\n");
        // }
        execute(*argc, args);
        free(argc);
        free(args);
        job_cnt++;
    }
    
    free(jobs);
}

int main(int argc, char *argv[]) 
{
    if (argc > 2) {
        my_print(error_message);
    }
    char cmd_buff[BUFFER_LEN];
    char *pinput;

    //batch mode
    if (argc == 2) {
        FILE *batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            my_print(error_message);
            // my_print("batch: ");
            exit(0);
        }

        while (1) {
            pinput = fgets(cmd_buff, BUFFER_LEN, batch_file);
            if (!pinput) {
                // my_print("pinput: ");
                exit(0);
            }

            // my_print("looping \n");
            //checking for too long
            int too_long = 1;
            for (int i = 0; i < 513; i++) {
                if (cmd_buff[i] == '\0') {
                    too_long = 0;
                    break;
                }
            }
            if (too_long) {
                my_print(cmd_buff);
                my_print(error_message);
                continue;
            }

            //checking for empty
            char *copy = strdup(cmd_buff);
            char *delims = "\t\n\r\v ";
            char *is_empty = strtok(copy, delims);

            if (is_empty != NULL) {
                my_print(cmd_buff);       
                run_cmd(cmd_buff);
            }
            free(copy);
        }

        fclose(batch_file);
        // my_print("hi");
        exit(0);
    }

    // interactive mode
    if (argc == 1) {
        while (1) {
            my_print("myshell> ");

            pinput = fgets(cmd_buff, BUFFER_LEN, stdin);

            //checking for too long
            int eof_found = 0;
            for (int i = 0; i < 513; i++) {
                if (cmd_buff[i] == '\0') {
                    eof_found = 1;
                    break;
                }
            }
            if (eof_found == 0) {
                my_print("input too long\n");
            }

            if (!pinput) {
                exit(0);
            }
            if (eof_found == 1) {
                run_cmd(cmd_buff);
            }
        }
    }
}
