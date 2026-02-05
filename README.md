# C++ Mini Shell

A minimal UNIX-like shell implemented in C++ using POSIX system calls.

## Features

- Command execution using `fork()` and `execvp()`
- Piping support (`|`)
- Background process execution (`&`)
- Built-in commands (cd, exit)
- Error handling and process synchronization

## Technologies

- C++
- POSIX System Calls
- Linux Environment

## Concepts Covered

- Process Creation
- Inter-Process Communication (IPC)
- File Descriptors
- Pipe Mechanism
- Wait & Process Synchronization

## Example

```bash
$ ls -l
$ ps aux | grep root
$ sleep 5 &
