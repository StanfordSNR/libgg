#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gg.h"

char *get_gg_file( const char *filename ){
  char env_name[100];
  strcpy(env_name, filename);

  fprintf(stderr, "DANITER DEBUG : %s\n", filename);

  for( char *c = env_name; *c != '\0'; c++){
    char* dot = strchr( c, '.' );
    if( dot != NULL ){
      *dot = '_';
      c = dot;
    }
    dot = strchr( c, '/' );
    if( dot != NULL ){
      *dot = '_';
      c = dot;
    }
  }
  return getenv( env_name );
}