#include "gg.h"

int get_gg_file(const char *filename, char * gg_file_out){
  gg_file_out = getenv(filename);
  return  gg_file_out != NULL ? 0 : 1;
}