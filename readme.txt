This allocator is implemented with reference to BIBOP memory management strategy.In this project, allocator is implemented based on the virtual memory allocated through mmap function. A 4K size page is called a bag.

1. Function
The allocator implemented the function of 4 standard C API:
malloc, calloc, realloc, free.


2. Core Idea Description
There are 2 types of memory allocation. The first one is that when the memory size is less than 1024 bytes, the memory will be allocated inside a bag. The other one is that when size larger than 1024 bytes, separate bags will be allocated.

Inside a bag, in front of the memory to be allocated for user, metadata exists to represent the organization information of each bag.

In particular, in the first condition, bags are maintained in the way of linked list. In case memory size larger than 1024 bytes, the metadata for the second condition records the number of pages to make memory release easier.


3. Run and Test the Allocator
Inside the package, there is a script which can substitute the system library with the source code in dynamic running circumstances, by using environment variable LD_PRELOAD.

To run the project, you just need to go to the path of Allocator package by command - cd Allocator, then insert the command - make run. 


 