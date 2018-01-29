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

struct MemoryRegion {
  void *startAddr;
  void *endAddr;
  char permission[4];
  unsigned long long length;
};

char ckpt_image[1000]; //file name
ucontext_t uc;
struct MemoryRegion mr;

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
  FILE* fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t lread;
  char *string, *address, *startAddr, *endAddr;
  unsigned long long start, end, length;
  if((fp = fopen("/proc/self/maps", "r")) < 0) {
    printf("Fail to open self maps file.\n");
    exit(EXIT_FAILURE);
  };
  while((lread = getline(&line, &len, fp)) != -1){
    if((string = strstr(line, "[stack]")) != NULL) {
      address = strtok(line, " ");
      startAddr = strtok(address, "-");
      endAddr = strtok(NULL, "\0");
      start = hex_to_ten(startAddr);
      end = hex_to_ten(endAddr);
      length = end - start;
    }
    //release the stack
    munmap((void*)startAddr, (size_t)length);
  }

  printf("Success to unmap!\n");
  fclose(fp);


  //Step2: restore the checkpoint from the file. First restore the data of memory,
  //then restore the context of process.
  struct MemoryRegion *mr;
  int ret;
  void* map;
  if((fp = fopen(ckpt_image, "r")) == NULL) {
    printf("Fail to open checkpoint file.\n");
    exit(EXIT_FAILURE);
  };
  mr = (struct MemoryRegion *)malloc(sizeof(struct MemoryRegion));

  while((ret = fread(mr, sizeof(struct MemoryRegion), 1, fp)) > 0) {
    //mmap the memory section
    if((map = mmap((void*)mr->startAddr, mr->length, PROT_READ|PROT_EXEC|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED , -1, 0)) < (void*)0) {
      printf("Fail to mmap memory section. Error: %s\n", strerror(errno));
    }
    printf("start: %s, length: %llu, permission: %s\n", (char*)mr->startAddr, mr->length, mr->permission);
    //set the permission
    int permissions = 0;
    if(mr->permission[0] != '-')
      permissions |= PROT_READ;
    if(mr->permission[1] != '-')
      permissions |= PROT_WRITE;
    if(mr->permission[2] != '-')
      permissions |= PROT_EXEC;
    //printf("start: %s, length: %llu, permission: %s\n", (char*)mr->startAddr, mr->length, mr->permission);
    if ((ret = mprotect(mr->startAddr, mr->length, permissions)) < 0 )
      printf("Fail to mprotect. Error: %s\n", strerror(errno));
    //restore the memory data
    if(fread(mr->startAddr, mr->length, 1, fp) < 0) {
      printf("Fail to read memory data. Error: %s\n", strerror(errno));
    };
  }
  printf("Success to restore memory data!\n");

  //Step3: restore the context
  if((ret = fread(&uc, sizeof(uc), 1, fp)) < 0) {
    printf("Fail to read context from checkpoint file.\n");
  }
  setcontext(&uc);
  printf("Success to restore context.\n");
  printf("Finish the whole work!!!\n");
  fclose(fp);
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
