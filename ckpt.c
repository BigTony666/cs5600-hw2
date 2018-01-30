#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>

struct MemoryRegion {
  void *startAddr;
  void *endAddr;
  char permission[4];
};

#define MAX_NUMBER_HEADER 50
#define MAPS_PATH "/proc/self/maps"
#define MAX_NUMBER_LINE_WORD 1000
int mrCount = 0;
int flag = 0;

//All the function definition
unsigned long long hex_to_ten(char *hex);
int saveToDisk(int fd_Write, struct MemoryRegion *mr);
int SaveCkpt();
void handler(int sign);

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
  int res;
  unsigned long long start, end, length = 0;
  int fd_Open, fd_Write, i = 0;
  char str[MAX_NUMBER_LINE_WORD];
  char ch, *string, *startAddr, *endAddr, *permission, *line;
  ucontext_t mycontext;
  //malloc a region for MemorySection  wait to clean
  //mrs = (struct MemoryRegion *)malloc(MAX_NUMBER_HEADER * sizeof(struct MemoryRegion));

  //open the maps file
  fd_Open = open(MAPS_PATH, O_RDONLY);
  fd_Write = open("./myckpt", O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
  //error
  if(fd_Open < 0) {
    printf("Failed to open maps file.\n");
    exit(EXIT_FAILURE);
  }
  if(fd_Write < 0) {
    printf("Failed to create myckpt file.\n");
    exit(EXIT_FAILURE);
  }

  //write context
  if (getcontext(&mycontext) < 0) {
    printf("Fail to get context.\n");
    return -1;
  }
  if(flag == 1) {
    return 0;
  } else {
    flag = 1;
  }

  res = write(fd_Write, &mycontext, sizeof(ucontext_t));
  if(res <= 0) {
    printf("Fail to save context.\n");
  } else {
    printf("Success to save context.\n");
  }


  //read line by line and extract information
  while((read(fd_Open, &ch, 1)) > 0){
    if(ch != '\n') {
      *(str + i) = ch;
      i++;
      continue;
    } else if(ch == '\n' || ch == '\0'){
      *(str + i) = '\0';
      i = 0;
    }
    line = strtok(str, "\n");
    line[strlen(line)] = '\0';

    // skip [vvar], [vdso], [vsyscall]
    if(strstr(line, "[vvar]") == NULL && strstr(line, "[vdso]") == NULL && strstr(line, "[vsyscall]") == NULL) {
      //count how many MemoryRegion(line)
      printf("The line is: %s\n", line);
      //get startAddr, endAddr, and permission
      mr = (struct MemoryRegion *)malloc(sizeof(struct MemoryRegion));
      startAddr = strtok(line, " ");
      string = strdup(startAddr);
      permission = strtok(NULL, " ");
      startAddr = strtok(string, "-");
      endAddr = strtok(NULL, "\0");
      //printf("|%s| |%s|\n", startAddr, endAdd      r);

      //calculate the length of Memory Address
      start = hex_to_ten(startAddr);
      end = hex_to_ten(endAddr);
      length = end - start;
      mr->startAddr = (void*)start;
      mr->endAddr = (void *)end;
      strcpy(mr->permission, permission);

      //print the header information
      printf("The Headers are:\n");
      printf("|%p| |%p| |%s| |%llu|\n", mr->startAddr, mr->endAddr, mr->permission, length);
      printf("%ld\n", (char*)mr->endAddr - (char*)mr->startAddr);

      //save header and memory data to file
      if(mr->permission[0] == 'r') {
        if((res = saveToDisk(fd_Write, mr)) != 0) {
          printf("Fail to save checkpoint image.\n");
          return -1;
        }
        else {
          //free mr
          free(mr);
          continue;
        }
      } else {
        printf("Denied to access, skip this memory section.\n");
        //free mr
        free(mr);
        continue;
      }
    }
  }
  printf("-------------Success to save checkpoint file.------------\n");
  close(fd_Open);
  close(fd_Write);
  return 0;
}

//save the ucontext
int saveToDisk(int fd_Write, struct MemoryRegion *mr) {
    unsigned long long val1, val2;
    val1 = write(fd_Write, mr, sizeof(struct MemoryRegion));
    printf("%llu\n", val1);
    //assert(val == 1)
    if(val1 <= 0) {
      printf("Fail to save header.\n");
      return -1;
    } else {
      printf("Success to save header.\n");
    }
    //printf("%p %d\n", mr->startAddr, mr->endAddr - mr->startAddr);
    val2 = write(fd_Write, mr->startAddr, (char*)mr->endAddr - (char*)mr->startAddr);
    printf("%llu\n", val2);
    if(val2 <= 0) {
      printf("Fail to save memory data.\n");
      mrCount++;
      return -1;
    } else {
      printf("Success to save memory data.\n");
    }
    return 0;
}

//Declaring constructor functions
__attribute__((constructor))
void myconstructor() {
  //get pid
  printf("The pid is: %d\n", getpid());
	signal(SIGUSR2, handler);
}


void handler(int sign) {
  signal(sign, SIG_DFL); // react with default behavior
  int res;
  if((res = SaveCkpt()) != 0) {
    printf("Fail!\n");
    return;
  }
  return;
}
// int main() {
//   SaveCkpt();
//   return 0;
// }
