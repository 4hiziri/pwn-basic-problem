#include <stdio.h>

void vuln() {
  int flag = 0;
  char buf[256];

  fgets(buf, 300, stdin);

  if (flag == 0) {
    printf("Failed!\n");
  } else {
    printf("You won FLAG!\n");
  }
}

int main(int argc, char** argv){
  vuln();  
  return 0;
}
