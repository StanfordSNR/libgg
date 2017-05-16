#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gg.h"

char *get_gg_file( const char *filename ){
  char env_name[4096];
  strcpy(env_name, filename);

  if( getenv( GG_VERBOSE ) ){
    fprintf(stderr, "DANITER DEBUG : %s\n", filename);
  }

  for( char *c = env_name; *c != '\0'; c++){
    if( *c == '.' || *c == '/' || *c == '-' ){
      *c = '_';
    }
  }
  
  if( getenv( GG_VERBOSE ) ){
    fprintf( stderr, "DANITER GET_GG_FILE ENVNAME: %s\n", env_name );
  }

  return getenv( env_name );
}
