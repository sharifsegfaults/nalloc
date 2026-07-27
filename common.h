#pragma once

static inline size_t max_size_n(const size_t *values, size_t count) {
  size_t max = values[0];
  for (size_t i = 1; i < count; ++i)
     if (values[i] > max) max = values[i];
  return max;
}

#define MAX(...)                                           \
  max_size_n((const size_t[]){__VA_ARGS__},                \
  sizeof((const size_t[]){__VA_ARGS__}) / sizeof(size_t))  \
