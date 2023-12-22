Compiling and Executing:   
gcc/clang -o sh shell.c hashmap.c        
  1) Interactive / REPL Mode: ./sh   
  2) Batch / Scripting Mode: ./sh file   
     
Design Features:      
- Homemade C hashmap and enum matches user jobs to create an extensible command database  
    - Currently has: pwd, cd, ls, rm, mkdir, cat, touch, exit
- Separates into jobs and then arguments (delimited by spaces or tabs etc)  
- Allows redirection (>) and advanced/concatenating redirection (+>)

Takeaways from this project:  
 1) Making a hashmap (auxiliary to the cmd database):
    - Hash Functions require not only key but hashmap capacity as input to properly distribute indices
    - Polynomial Rolling Hash Function (string -> int):
        - a) Multiply current hash value by a prime (31 in the shell)
        - b) Add next char ASCII
        - c) Modulo capacity
    - Linked list to chain at collisions (could also resize)
    - Useful to have free_struct in your C API
        - However, freeing will still have to be done outside of the free method (e.g. hashMapRemove)
 2) -g flag for debugging info and -fsanitize=address (or =leak specifically) for memory bug info while compiling
    - to-do in the future: Learn how to properly use GDB, LLVM, and VSCode's debugger
 3) C is a full of status codes + conventions and written in a very "side-effecty" way
    - e.g. the primary computations we want to carry out are often the "boolean" expressions in a conditional
 4) Pattern: while (fgets(buffer, buff_len, file) != NULL) { ... do stuff ... }
    - need to take multiple trips to get stuff into the buffer
 5) General Coding Approach: Define API overview and declare as far up top as possible then implement
 6) Interesting strtok bug:
    - I tried to use strtok concurrently in two separate functions
        - Outer: strtok to tokenize jobs delimited by ';\t\n\r\v'
        - Inner: strtok to tokenize arguments given a job delimited by ' '
    - However, strtok is not reentrant i.e. it should not be used by two concurrent threads
        - strtok uses a single save variable to save where it has gotten on a string so far
    - Fix: use strtok_r() which requires you to pass in a manually allocated save variable (remember to free)
 7) Redirection vs Advanced Redirection:
    - add O_APPEND to the open syscall for advanced redirection, use O_CREAT to create files that don't exist yet
    - e.g. open(filename, flags, 0644);
        - 0 means octal, remaining three digits are in octal permission notation for owner, group, and other
    - Procedure for redirection:
        -save the STDOUT_FILENO file descriptor ID as a temporary integer
        - use dup2 to change STDOUT_FILENO to the desired fd
        - change STDOUT_FILENO back to standard out file descriptor via dup2 again
        - close your temporary stdout_copy and fd
 8) Most of my shell functions were just simple syscalls or prebuilt executables
    - e.g. chdir() syscall (combined with getenv()) for cd
    - e.g. exit() syscall for exit
    - e.g. getcwd() syscall for pwd
    - e.g. mkdir() syscall for mkdir
    - e.g. execvp("ls") process for ls
    - e.g. read(fd) and write(STDOUT_FILENO) syscalls for cat
    - e.g. just open() syscall for touch
 9) Pattern: fork() -> child execvp() and parent waitpid()    
  - had to use concurrent processes for ls
        - since not a syscall, need to create a child and run execvp("ls", args) or else will terminate shell
        - "ls" is a prebuilt executable
 10) Recursive rm: the most advanced component of the shell
  - DIR type: used with opendir(), closedir(), and readdir() e.g. DIR *dir = opendir(dirname);
  - struct dirent: directory entry
      - openining: struct dirent *entry = readdir(dir);
      - members such as d_name, d_ino (inode number)
  - struct stat: structure used to store loads of statistics about a file such as st_mode, st_size etc. 
      - struct stat path_stat; to declare, then: stat(path, &path_stat); where path is a string holding path name
      - S_ISDIR(path_stat.st_mode) tells us if the path is a directory
  - rmdir(directory_name) syscall for directories and unlink(file_path) for files
