# Project: Custom Memory Allocation

My first C project, a custom implementation of malloc.

## Overview

CustomMalloc is a function that uses its own structure to store the relevant data
required to keep track of the allocated memory.
The objective is to minimize calls to the native malloc method while maintaining 
a reasonable usage of memory.

The following is an example of a simplified diagram demonstrating the structure used by CustomMalloc :

![Memory Model Diagram](https://raw.githubusercontent.com/inHandy/CustomMemoryAllocation/master/images/Diagram1.png)

In addition, it is also possible to change the value of the "MEMBLOCKSIZE " macro in order
to customize the memory size of each MemBlock.
