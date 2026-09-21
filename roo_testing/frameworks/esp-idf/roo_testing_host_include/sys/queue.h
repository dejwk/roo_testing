#pragma once

// ESP-IDF's Linux component normally selects libbsd's queue macros. Keep the
// host build self-contained when libbsd-dev is unavailable by extending the
// platform's sys/queue.h with the one BSD traversal helper used by ESP-IDF.
#include_next <sys/queue.h>

#ifndef SLIST_FOREACH_SAFE
#define SLIST_FOREACH_SAFE(var, head, field, temp)                         \
  for ((var) = SLIST_FIRST(head);                                         \
       (var) != NULL && ((temp) = SLIST_NEXT(var, field), 1);             \
       (var) = (temp))
#endif
