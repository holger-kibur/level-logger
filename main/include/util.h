#ifndef UTIL_H
#define UTIL_H

#include "esp_log.h"
#include <errno.h>

// POSIX error check
#define POSIX_EC(X)                                                   \
  {                                                                            \
    if ((X) < 0) {                                                               \
      ESP_LOGE(TAG, "POSIX error check failed!\nCode:%s\nMessage: %s", errno,  \
               strerror(errno));                                               \
      abort();                                                                 \
    }                                                                          \
  }

// NULL pointer check
#define NPC(X)                                                      \
  {                                                                            \
    if (X == NULL) {                                                           \
      ESP_LOGE(TAG, "NULL pointer check failed!");                             \
      abort();                                                                 \
    }                                                                          \
  }

// Not NULL pointer check
#define NOT_NPC(X)                                                      \
  {                                                                            \
    if (X != NULL) {                                                           \
      ESP_LOGE(TAG, "Not NULL pointer check failed!");                             \
      abort();                                                                 \
    }                                                                          \
  }

// ESP_ERROR_CHECK
#define ESP_EC ESP_ERROR_CHECK

#endif // UTIL_H
