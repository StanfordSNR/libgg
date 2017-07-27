#ifndef _GG_H_
#define _GG_H_

#include <limits.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "util/vector.h"

void   __gg_read_thunk();
char * __gg_get_filename( const char * filename );
int    __gg_stat( const char * filename, struct stat * restrict buf );
bool   __gg_is_allowed( const char * filename, const bool check_infiles );
void   __gg_disable_infile( const char * filename );
char * __gg_normalize_path( const char * pathname, char * base );

typedef struct
{
  char gg_path[ PATH_MAX ];

  char filename[ PATH_MAX ];
  char hash[ 64 + 1 ];
  int order;
  off_t size;
  bool enabled;
} InFile;

typedef struct
{
  char path[ PATH_MAX ];
} InDir;

typedef InDir AllowedFile;

VECTORDEF( InFile );
VECTORDEF( InDir );
VECTORDEF( AllowedFile );

typedef struct
{
	bool enabled;
	bool verbose;
	char * dir;
	char * thunk_file;

	vector_InFile infiles;
	vector_InDir indirs;
  vector_AllowedFile allowed_files;
	char outfile[PATH_MAX];
  bool outfile_created;
} __gg_struct;

extern __gg_struct __gg;

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
