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
  char permission[4];
  unsigned long long length;
};

#define MAX_NUMBER_HEADER 50
#define MAPS_PATH "/proc/self/maps"

//All the function definition
unsigned long long hex_to_ten(char *hex);
int saveToDisk(FILE *fw, struct MemoryRegion *mr);
int SaveCkpt();
void handler();

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

int SaveCkpt(){
  struct MemoryRegion *mr;
  int mrCount = 0, res;
  unsigned long long length, start, end;
  FILE* fp, *fw;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char *string, *startAddr, *endAddr, *permission;
  //malloc a region for MemorySection  wait to clean
  //mrs = (struct MemoryRegion *)malloc(MAX_NUMBER_HEADER * sizeof(struct MemoryRegion));

  //open the maps file
  fp = fopen(MAPS_PATH, "r");
  fw = fopen("./myckpt", "w");
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

    //calculate the length of Memory Address
    start = hex_to_ten(startAddr);
    end = hex_to_ten(endAddr);
    length = end - start;
    mr->startAddr = startAddr;
    mr->endAddr = endAddr;
    strcpy(mr->permission, permission);
    mr->length = length;

    //print the header information
    printf("The Headers are:\n");
    printf("%s %s %s %llu\n", (char*)mr->startAddr, (char*)mr->endAddr, mr->permission, mr->length);

    if((res = saveToDisk(fw, mr)) < 0) {
      printf("Fail to save checkpoint image.\n");
      return -1;
    }
  }
  //free mr
  free(mr);
  //free the line
  if(line) {
    free(line);
  }
  printf("Success save checkpoint image.\n");
  return 0;
}

//save the ucontext
int saveToDisk(FILE *fw, struct MemoryRegion *mr) {
  ucontext_t mycontext;
  int val;
  if (getcontext(&mycontext) < 0) {
    printf("Fail to get context.\n");
    return -1;
  }
  val = fwrite(mr, sizeof(struct MemoryRegion), 1, fw);
  if(val != 1) {
    printf("Fail to save header.\n");
  }
  val = fwrite((void*)mr->startAddr, mr->length, 1, fw);
  if(val != 1) {
    printf("Fail to save memory data.\n");
  }
  val = fwrite(&mycontext, sizeof(mycontext), 1, fw);
  if(val != 1) {
    printf("Fail to save context.\n");
  }
  return 0;
}

//Declaring constructor functions
__attribute__ ((constructor))
void myconstructor() {
  signal(SIGUSR2, handler);
}

void handler(int sign) {
  signal(sign, SIG_DFL);
  int res;
  if((res = SaveCkpt()) != 0) {
    printf("Fail!\n");
    return;
  }
  return;
}
