#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <ucontext.h>
#include <errno.h>

#define ADDRESS 0x5300000
#define SIZE 0x1000
#define MAPS_PATH "/proc/self/maps"
#define MAX_NUMBER_LINE_WORD 1000

struct MemoryRegion {
  void *startAddr;
  void *endAddr;
  char permission[4];
};

char ckpt_image[1000]; //file name
ucontext_t uc;

//all the functions are here
void restore_memory();
unsigned long long hex_to_ten(char *hex);

int main(int argc, char* argv[]) {
  if(argc == 1) {
    printf("Please add file name.\n");
    return -1;
  }
  strcpy(ckpt_image, argv[1]);
  //Map in some new memort for a new stack
  void *p_map;
  p_map = mmap((void *)ADDRESS, SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
  void *stack_ptr;
  stack_ptr = (void*)(p_map + 0x1000);

  //Use the inline assembly syntax of gcc to include code that will switch the stack pointer
  asm volatile ("mov %0,%%rsp" : : "g" (stack_ptr) : "memory");
  restore_memory();
  printf("Restore Success!\n");
  return 0;
}

void restore_memory() {
  //Step1: remove the current stack of program myrestart using ummap
  int fd, i = 0;
  char *line, *address, *startAddr, *endAddr, ch;
  char str[MAX_NUMBER_LINE_WORD];
  unsigned long long start, end;
  long int length = 0;
  if((fd = open(MAPS_PATH, O_RDONLY)) < 0) {
    printf("Fail to open self maps file.\n");
    exit(EXIT_FAILURE);
  };
  while((read(fd, &ch, 1)) > 0){
    if(ch != '\n') {
      *(str + i) = ch;
      i++;
      continue;
    } else {
      *(str + i) = '\0';
      i = 0;
    }
    line = strtok(str, "\n");
    line[strlen(line)] = '\0';

    //find the stack memory section and munmap it
    if((strstr(line, "[stack]")) != NULL) {
      address = strtok(line, " ");
      startAddr = strtok(address, "-");
      endAddr = strtok(NULL, "\0");
      start = hex_to_ten(startAddr);
      end = hex_to_ten(endAddr);
      length = end - start;

      //release the stack
      munmap((void*)startAddr, (size_t)length);
      printf("Success to unmap!\n");
    }
  }
  close(fd);


  //Step2: restore the memory data and context from the checkpoint file.
  struct MemoryRegion *mr;
  int ret;
  void* map;
  if((fd = open(ckpt_image, O_RDONLY)) < 0) {
    printf("Fail to open checkpoint file.\n");
    exit(EXIT_FAILURE);
  };

  //read the context first and store it into uc
  if((read(fd, &uc, sizeof(ucontext_t))) <= 0) {
    printf("Fail to read context from checkpoint file.\n");
  }

  mr = (struct MemoryRegion *)malloc(sizeof(struct MemoryRegion));

  while((read(fd, mr, sizeof(struct MemoryRegion))) > 0) {
    //mmap the memory section
    length = (char*)mr->endAddr - (char*)mr->startAddr;
    printf("startAddr:%p length: %ld\n", mr->startAddr, length);
    if((map = mmap((void*)mr->startAddr, length,
       PROT_READ|PROT_EXEC|PROT_WRITE,
       MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED , -1, 0)) < 0) {
      printf("Fail to mmap memory section. Error: %s\n", strerror(errno));
    } else {
      printf("Success to mmap memory section!\n");
    }
    //printf("start: %s, length: %llu, permission: %s\n", (char*)mr->startAddr, mr->length, mr->permission);

    //restore the memory data
    mprotect(mr->startAddr, length, PROT_WRITE);
    printf("startAddr:%p length: %ld\n", mr->startAddr, (char*)mr->endAddr - (char*)mr->startAddr);
    if(read(fd, mr->startAddr, (char*)mr->endAddr - (char*)mr->startAddr) < 0) {
      printf("Fail to read memory data. Error: %s\n", strerror(errno));
    } else {
      printf("Success to read memory section data!\n");
    }

    //set the permission
    int permissions = 0;
    if(mr->permission[0] != '-')
      permissions |= PROT_READ;
    if(mr->permission[1] != '-')
      permissions |= PROT_WRITE;
    if(mr->permission[2] != '-')
      permissions |= PROT_EXEC;
    if ((ret = mprotect(mr->startAddr, length, permissions)) < 0 )
      printf("Fail to mprotect. Error: %s\n", strerror(errno));
    else
      printf("Success to mprotect memory section!\n");
    free(mr);
    mr = (struct MemoryRegion *)malloc(sizeof(struct MemoryRegion));
  }
  printf("Success to restore all memory data!\n");
  printf("-----Finish the whole work!!!-----\n");
  free(mr);
  //Step3: restore the context
  setcontext(&uc);
  close(fd);
  return;
}

//convert hex string into ten base number
unsigned long long hex_to_ten(char *hex) {
  int i;
  char c;
  unsigned long long value = 0;
  int length = strlen(hex);
  for(i = 0; i < length; i++) {
    c = *(hex + i);
    if ((c >= '0') && (c <= '9')) c -= '0';
    else if ((c >= 'a') && (c <= 'f')) c -= 'a' - 10;
    else if ((c >= 'A') && (c <= 'F')) c -= 'A' - 10;
    value = value * 16 + c;
  }
  return value;
}
