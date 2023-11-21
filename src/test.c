#include <stdio.h>

int foo() {
  // printf("Hello, Foo!");
  for (int i = 0; i < 10000; i++) {
    int j = 0;
  }
  return 0;
}

int bar() {
  // printf("Hello, Bar!");
  for (int i = 0; i < 10000; i++) {
    int j = 0;
  }
  return 0;
}

int baz() {
  // printf("Hello, Baz!");
  for (int i = 0; i < 100000; i++) {
    if (i % 3 == 0) {
      foo();
    } else {
      bar();
    }
  }
  return 0;
}

int main() {
  baz();
  return 0;
}