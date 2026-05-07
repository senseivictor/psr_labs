#include <string.h>

const char *get_content_type(char *filename) {
  if (strstr(filename, ".html") || strstr(filename, ".htm")) {
    return "text/html";
  } else if (strstr(filename, ".png")) {
    return "image/png";
  } else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) {
    return "image/jpeg";
  } else if (strstr(filename, ".gif")) {
    return "image/gif";
  } else if (strstr(filename, ".txt")) {
    return "text/plain";
  }
  return "application/octet-stream"; // Tip generic pentru fisiere binare
}