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
- 1) Making a hashmap (auxiliary to the cmd database):
    - Hash Functions require not only key but hashmap capacity as input to properly distribute indices
    - Polynomial Rolling Hash Function (string -> int):
        - a) Multiply current hash value by a prime (31 in the shell)
        - b) Add next char ASCII
        - c) Modulo capacity
    - Linked list to chain at collisions (could also resize)
    - Useful to have free_struct in your C API
        - However, freeing will still have to be done outside of the free method (e.g. hashMapRemove)
- 2) -g flag for debugging info and -fsanitize=address (or =leak specifically) for memory bug info while compiling
    - to-do in the future: Learn how to properly use GDB, LLVM, and VSCode's debugger
- 3) C is a full of status codes + conventions and written in a very "side-effecty" way
    - e.g. the primary computations we want to carry out are often the "boolean" expressions in a conditional
- 4) execvp, waitpid, fork()    
- specifically, the implementation of ls: not a syscall, need to create a child and run execvp("ls", args)    
- implementation of redir and advanced redirection
- Cleaning up after myself and using -fsanitize=address flag and freeing properly
- Issue: Using strtok on two different strings
 
