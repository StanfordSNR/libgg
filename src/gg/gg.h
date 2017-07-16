#ifndef _GG_H_
#define _GG_H_

#include <limits.h>
#include <stdbool.h>

#include "util/vector.h"

void __gg_read_thunk();
char * get_gg_file( const char * filename );
int is_dir_allowed( const char * path );

typedef struct
{
  char gg_path[ PATH_MAX ];

  char filename[ PATH_MAX ];
  char hash[ 64 + 1 ];
  int order;
} InFile;

typedef struct {
  char path[ PATH_MAX ];
} InDir;

VECTORDEF( InFile );
VECTORDEF( InDir );

extern bool __gg_verbose;
extern bool __gg_enabled;

extern char * __gg_dir;
extern char * __gg_thunk;

extern vector_InFile infiles;
extern vector_InDir indirs;
extern char __gg_outfile[ PATH_MAX ];

#define GG_INFO( ... )    fprintf( stderr, "[gg:info] " __VA_ARGS__ )
#define GG_DEBUG( ... )   fprintf( stderr, "[gg:debug] " __VA_ARGS__ )
#define GG_WARNING( ... ) fprintf( stderr, "[gg:warning] " __VA_ARGS__ )
#define GG_ERROR( ... )   fprintf( stderr, "[gg:error] " __VA_ARGS__ )

#define GG_THUNK_MAGIC_NUMBER "##GGTHUNK##"

#define GG_THUNK_PATH_ENVAR "__GG_THUNK_PATH__"
#define GG_ENABLED_ENVAR    "__GG_ENABLED__"
#define GG_DIR_ENVAR        "__GG_DIR__"
#define GG_VERBOSE_ENVAR    "__GG_VERBOSE__"

#endif
