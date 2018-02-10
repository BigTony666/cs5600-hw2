# mini-DMTCP
DMTCP is a widely used software package for checkpoint-restart: http://dmtcp.sourceforge.net. This program is to implement a basic
process checkpoint saving and restoring function on Linux. 

# Design Method
The implementation is mainly using mmap(), munmap(), write(), open(), read(), setcontext(), getcontext() system calls.
- In saving step, the context and memory value in process is saved by calling getcontext(), reading /proc/PID/maps file and then
writing to the binary file.
- In restoring step, the context is restored by setcontext() and memory is restored by reading checkpoint file and calling mmap().

# Building and Testing
- Build:
  ```sh
  Use make clean : to clear the whole
  Use make gdb : to debug
  ```

- Run:
   ```sh
  Use make : to run the first step of program(save the checkpoint file and kill the old program)
  Use make res : to run the second step of program(restore process from myskpt file)
  ```
