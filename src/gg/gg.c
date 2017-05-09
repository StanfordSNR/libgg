#include <stdlib.h>
#include "gg.h"

char *get_gg_file( const char *filename ){
  return getenv( filename );
}