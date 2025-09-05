#include <stdio.h>
#include <unistd.h>

int main() {
  alarm(1);

  int i = 0;

  while (1) {
    printf("%i\n", i++);
  }
  return 0;
}