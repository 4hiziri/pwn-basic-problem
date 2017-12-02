#include <stdio.h>
#include <unistd.h>

void shell() {
  char* args[] = {"/bin/sh", NULL};
  execve("/bin/sh", args,  NULL);
}

void echo() {
  char buf[0x100];

  fgets(buf, 0x200, stdin);

  puts(buf);

  return;
}

int main(int argc, char** argv){
  while(1) {
    echo();
  }
  
  return 0;
}
