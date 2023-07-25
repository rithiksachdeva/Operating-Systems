# UC Davis ECS150 Operating Systems - Fall 2022
[![Build Status](https://travis-ci.org/joemccann/dillinger.svg?branch=master)](https://travis-ci.org/joemccann/dillinger)
## Project 1 Summary: Simple Shell (sshell)
This project aims to implement a simple UNIX shell named sshell. The primary goal is to explore UNIX system calls related to processes, files, and pipes, while honing our skills in C programming, following established industry standards.
### Objectives
- Reviewing key concepts from previous programming courses, such as data structures, file manipulation, command line arguments, and Makefile.
- Discovering and utilizing various system calls that UNIX-like operating systems typically provide.
- Gaining a comprehensive understanding of how a shell works and how processes are launched and configured.
### Core Features
- Execution of user-supplied commands with optional arguments
- Selection of typical built-in commands
- Redirection of the standard output of commands to files
- Composition of commands via piping
### Extra Features
- Redirection of a file to a command’s standard input
- Stack of remembered directories

## Project 2 Summary: User-level Thread Library
This project aims to implement a user-level thread library for Linux to understand the idea of threads. The library must provide a complete interface for applications to create and run independent threads concurrently, test the code by writing custom testers, and maximize the test coverage. This project is also an opportunity to delve deeper into C programming, following established industry standards.
### Objectives
- Implement a queue/list, a common container used in system programming, as specified by a given API.
- Learn code testing by writing personalized testers and maximizing test coverage.
- Understand how multiple threads can run within the same process, from creation, concurrent execution, context switching, to termination.
- Implement a semaphore, a popular synchronization primitive, as specified by a given API.
- Write high-quality C code by following established industry standards.
### Core Features
- Ability to create new threads and schedule their execution in a round-robin fashion.
- An interrupt-based scheduler for preemption.
- A thread synchronization API, particularly semaphores.
- A queue API, allowing for O(1) operations apart from iteration and deletion.
- A uthread API, allowing for the management and manipulation of threads.
### Extra Features
- Semaphore API implementation, ensuring the correct and fair allocation of resources among threads.
- A preemption feature, allowing for the interruption of any thread to schedule another one, thus preventing the hogging of resources.
### Key Modules & Phases
1. Queue API Implementation: Implement a simple FIFO queue, ensuring all operations (apart from iteration and deletion) are O(1).
2. Uthread API Implementation: Implement thread management, allowing for the creation, termination, and manipulation of threads. Non-preemptive scheduling to be initially implemented.
3. Semaphore API Implementation: Implement semaphore API to control the access to common resources by multiple threads.
4. Preemption Implementation: Add a preemption feature, allowing for the interruption of threads to schedule another one, preventing resource hogging. The function should be completely transparent to user threads and only be enabled when user code is running.


## Project 3 Summary: Virtual File System
This project involves creating an entire FAT-based filesystem software stack, from mounting and unmounting a formatted partition, to reading, writing, creating, and removing files. The goal is to understand how a formatted partition can be emulated in software using a simple binary file, without low-level access to an actual storage device.
### Objectives
- Implement a full FAT-based filesystem software stack.
- Understand how to emulate a formatted partition in software.
- Learn how to test code by writing custom testers and maximizing test coverage.
- Write high-quality C code by following established industry standards.
### Core Features
- Mounting and unmounting a virtual disk containing the file system.
- Creating and deleting files.
- Managing file descriptors, including opening and closing files.
- Reading from and writing to files.
### Architecture
The filesystem is implemented on a virtual disk and is based on a FAT (File Allocation Table) structure, supporting up to 128 files in a single root directory. The file system has four logical parts:
1. Superblock: Contains information about the file system (number of blocks, size of the FAT, etc.)
2. File Allocation Table (FAT): Keeps track of free data blocks and the mapping between files and the data blocks holding their content.
3. Root directory: Contains an entry for each file of the file system, defining its name, size, and the location of the first data block for the file.
4. Data blocks: Used by the content of files.
### Suggested Work Phases
1. Phase 1 (Mounting/Unmounting): Implement fs_mount() and fs_umount(), ensuring that the virtual disk is properly closed and all internal data structures of the FS layer are properly cleaned.
2. Phase 2 (File Creation/Deletion): Implement fs_create() and fs_delete() to add or remove files from the file system.
3. Phase 3 (File Descriptor Operations): Implement fs_open(), fs_close(), and fs_lseek() for opening, closing, and setting the file offset. Implement fs_stat() to return the file size.
4. Phase 4 (File Reading/Writing): Implement fs_read() and fs_write() for reading from and writing to a file.
