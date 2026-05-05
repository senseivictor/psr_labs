#include <errno.h>
#include <stdio.h>

// Macro pentru afișarea erorilor
#define ERROR(s)                                                               \
  {                                                                            \
    fprintf(stderr, "%d-", errno);                                             \
    perror(s);                                                                 \
    return (-1);                                                               \
  }
