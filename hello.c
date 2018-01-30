#include <stdio.h>
#include <unistd.h>

int main() {
  int sleep_time = 1;

  while(1){
    printf(". ");
    //force the output Normally
    fflush(stdout);
    //sleep for one second
    sleep(sleep_time);
  }
  return 0;
}
