#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct MemoryRegion {
  void *startAddr;
  void *endAddr;
  int isReadable;
  int isWriteable;
  int isExecutable;
};

#define MAX_NUMBER_HEADER 50
#define MAPS_PATH "/proc/self/maps"

//All the function definition
unsigned long long hex_to_ten(char *hex);


//convert hex string into ten base number
unsigned long long hex_to_ten(char *hex) {
  int i;
  char c;
  unsigned long long value = 0;
  int length = strlen(hex);
  for(i = 0; i < length; i++) {
    c = hex[i];
    if ((c >= '0') && (c <= '9')) c -= '0';
    else if ((c >= 'a') && (c <= 'f')) c -= 'a' - 10;
    else if ((c >= 'A') && (c <= 'F')) c -= 'A' - 10;
    value = value * 16 + c;
  }
  return value;
}

int main(){
  struct MemoryRegion *mr;
  int mrCount = 0;
  unsigned long length;
  FILE* fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char *string, *startAddr, *endAddr, *permission;
  //malloc a region for MemorySection  wait to clean
  //mrs = (struct MemoryRegion *)malloc(MAX_NUMBER_HEADER * sizeof(struct MemoryRegion));

  //open the maps file
  fp = fopen(MAPS_PATH, "r");
  //error
  if(fp == NULL) {
    printf("Failed to open that file.\n");
    exit(EXIT_FAILURE);
  }

  //read line by line and extract information
  while((read = getline(&line, &len, fp)) != -1){
    //count how many MemoryRegion(line)
    mrCount++;
    //get startAddr, endAddr, and permission
    mr = (struct MemoryRegion *)malloc(sizeof(struct MemoryRegion));
    startAddr = strtok(line, " ");
    string = strdup(startAddr);
    permission = strtok(NULL, " ");
    startAddr = strtok(string, "-");
    endAddr = strtok(NULL, "\0");
    printf("%s %s %s\n", startAddr, endAddr, permission);

    length = hex_to_ten(startAddr) - hex_to_ten(endAddr);
    mr->startAddr = startAddr;
    mr->endAddr = endAddr;

  }

  //free the line
  if(line) {
    free(line);
  }
  return 0;
}
