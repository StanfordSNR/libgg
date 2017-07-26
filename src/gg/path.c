#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define IS_DELIM(x) ( x == '\0' || x == '/' )

char * normalize_path( const char * pathname, char * base )
{
  char source_path[ PATH_MAX ] = { 0 };

  if ( pathname[ 0 ] != '/' ) {
    if ( base != NULL ) {
      strcpy( source_path, base );
      strcat( source_path, "/" );
    }

    strcat( source_path, pathname );
  }
  else {
    strcpy( source_path, pathname );
  }

  char * normalized = malloc( PATH_MAX * sizeof( char ) );
  memset( normalized, '\0', PATH_MAX );

  size_t slash_stack[ PATH_MAX ];
  size_t stack_head = 0;
  size_t ni = 0;

  bool absolute_path = ( source_path[ 0 ] == '/' );

  if ( absolute_path ) {
    strcat( normalized, "/" );
    ni++;
    slash_stack[ stack_head++ ] = ni;
  }

  char * saveptr;
  char * component = strtok_r( source_path, "/", &saveptr );

  while ( !absolute_path && component != NULL && strcmp( component, ".." ) == 0 ) {
    strcat( normalized, "../" );
    ni += strlen( "../" );

    component = strtok_r( NULL, "/", &saveptr );
  }

  while ( component != NULL ) {
    if ( strcmp( component, ".." ) == 0 ) {
      if ( !absolute_path && stack_head == 0 ) {
        strcat( normalized, "../" );
        ni += strlen( "../" );
      }
      else if ( stack_head != 0 ) {
        ni = slash_stack[ --stack_head ];
        normalized[ ni ] = '\0';
      }
    }
    else if ( strcmp( component, "." ) != 0 ) {
      slash_stack[ stack_head++ ] = ni;
      strcat( normalized, component );
      strcat( normalized, "/" );

      ni += strlen( component ) + 1;
    }

    component = strtok_r( NULL, "/", &saveptr );
  }

  if ( normalized[ ni - 1 ] == '/' ) {
    normalized[ ni - 1 ] = '\0';
    ni--;
  }

  normalized = realloc( normalized, ni + 1 );

  return normalized;
}
