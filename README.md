link/compile: gcc -o sh shell.c hashmap.c     

A simple shell with two modes:   
  1) Interactive Mode: REPL  (./sh)
  2) Batch Mode: Scripts (./sh file)

Design Features   
- Homemade C hashmap and enum matches user jobs to create an extensible command database  
- Separates into jobs (delimited by ;) and then arguments (delimited by spaces or tabs etc)  
- Handles redirection  

Learnings from this project:  
- Custom Data Structures in C  
- execvp, waitpid, fork()    
- specifically, the implementation of ls: not a syscall, need to create a child and run execvp("ls", args)    
- implementation of redir  

  TODO
 
