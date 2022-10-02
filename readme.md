# Slab memory allocator

## Project summary
An implementation of a kernel memory allocator. FrontEnd consists of a Slab Allocator, while the backend implements a Buddy Allocator.

## Some important notes
* the project is written in C
* the project is tested using Microsoft Visual Studio
* the implementation is windows only, because it contains concurrency sync logic specific to windows
* everything is thread safe, sync is done using semaphores from the `<windows.h>` header file
* for the project to work, the allocator must first be initialized using the `kmem_init(void*, int)` function
* the main file contains a simple test used to check core functionality

## Usage
- clone the project
    ``` bash
    git clone https://github.com/mateja-filipovic/Buddy-Slab-Allocator.git
    ```
- compile the project
- run the executable of the `main.c` file, that contains a simple test
- The test checks functionalities like creating caches, shrinking them, allocation and deallocation of objects as well as error checking