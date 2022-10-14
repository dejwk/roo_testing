#include <stdlib.h>
#include <stdbool.h>

char* itoa(int num, char* str, int base) {
  int i = 0;
  bool neg = false;

  if (num == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return str;
  }

  if (num < 0 && base == 10) {
    neg = true;
    num = -num;
  }

  while (num != 0) {
    int rem = num % base;
    num /= base;
    str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
  }

  if (neg) str[i++] = '-';

  str[i] = '\0';
  char* s = str;
  char* r = str + i - 1;
  while (s < r) {
    char tmp = *s;
    *s = *r;
    *r = tmp;
    s++;
    r--;
  }

  return str;
}
