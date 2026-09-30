#include "sum.h"

long long Sum(const struct SumArgs *args) {
  long long sum = 0;

  for (uint32_t i = args->begin; i < args->end; i++) {
    sum += args->array[i];
  }

  return sum;
}