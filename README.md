# OS_Shell_Project
A custom Unix-like shell implemented in C, supporting basic command execution, piping, redirection, signal handling, command history, and multiple command operations.

*Folder `tests` contains some files of each of the features and the `My_Shell.c` has the whole project in it*

🔧 Features

    Custom Command Prompt
    Displays a prompt (e.g., sh>) and reads user input.

    Command Execution
    Supports execution of standard Linux commands using fork and exec system calls.

    Input and Output Redirection
    Allows reading from files using <, writing output using >, and appending output using >>. Implements redirection with dup and dup2.

    Command Piping
    Supports connecting commands using | and allows multiple pipes in a single line.

    Multiple Command Handling
    Enables running multiple commands separated by semicolon (;) and conditional execution using logical AND (&&).

    Command History
    Stores a history of previously executed commands for reference.

    Signal Handling
    Handles interrupts like Ctrl+C to stop only the running foreground process, not the shell itself.

![Image](https://github.com/user-attachments/assets/bf2eb6e4-f313-4d62-ae04-f066495ef95b)
![Image](https://github.com/user-attachments/assets/7c24b245-e2be-4cd6-b815-de1ee285b390)

This shell is built entirely for terminal-based use on Linux systems and demonstrates concepts of process management, inter-process communication, and low-level system programming in C.
