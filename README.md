Introduction
----------
This project is a university assignment where I've created a miniature shell using the C programming language. The shell supports fundamental Unix commands such as ls, mkdir, touch, alongside more advanced features including the implementation of pipelines and the ability to run jobs in the background, with the option to display them.
### Features
    Basic Unix commands support: ls, mkdir, touch, etc.
    Implementation of pipelines for executing commands sequentially.
    Background job execution capability.
    Display of running background jobs.
Compilation and execution
----------
cd ensimag-shell
cd build
cmake ..
make
./ensishell

Commands to execute
----------
1- ls
2- mkdir
3- cmd & (background processes)
4- jobs ( to see current processes running)
5- cmd 1 | cmd2 (pipelines)
