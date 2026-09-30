#ifndef SUM_H
#define SUM_H

#include <stdint.h>

struct SumArgs {
  int *array;
  uint32_t begin;
  uint32_t end;
  long long result;
};

long long Sum(const struct SumArgs *args);

#endif