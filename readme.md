# Slab memory allocator
## Description
This is a university course project for Operating Systems 2. It's am implementation of the Slab allocator, which uses the buddy allocator as a backend.
## Some important notes
* the project is tested using Microsoft Visual Studio
* everything is thread safe, sync is done using semaphores from the windows.h header file
* for the project to work, the allocator must first be initialized using the kmem_init function
* the main file contains a simple test used to check core functionality