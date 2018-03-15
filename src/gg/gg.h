#ifndef _GG_H_
#define _GG_H_

#include <limits.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "util/vector.h"

#define HASH_MAX_LENGTH 65

typedef struct
{
  char gg_path[ PATH_MAX ];

  char filename[ PATH_MAX ];
  char hash[ HASH_MAX_LENGTH ];
  bool enabled;

  off_t size;
} InData;

typedef struct
{
  char path[ PATH_MAX ];
} DummyDir;

typedef struct
{
  char path[ PATH_MAX ];
  char target[ PATH_MAX ];
} AllowedFile;

typedef struct
{
  char filename[ PATH_MAX ];
  char tag[ PATH_MAX ];
  bool created;
} Output;

void     __gg_read_thunk();
void     __gg_read_manifest();
char *   __gg_get_filename( const char * filename );
int      __gg_stat( const char * filename, struct stat * restrict buf );
char *   __gg_get_allowed( const char * filename, const bool check_infiles );
char *   __gg_create_allowed( const char * filename );
Output * __gg_get_output( const char * filename );
void     __gg_disable_infile( const char * filename );
char *   __gg_normalize_path( const char * pathname, char * base );

VECTORDEF( InData );
VECTORDEF( DummyDir );
VECTORDEF( AllowedFile );
VECTORDEF( Output );

typedef struct
{
  bool enabled;
  bool verbose;
  char * dir;
  char * thunk_file;
  char * manifest_file;

  vector_InData indata;
  vector_DummyDir indirs;
  vector_AllowedFile allowed_files;
  vector_Output outputs;
} __gg_struct;

extern __gg_struct __gg;

#define GG_THUNK_MAGIC_NUMBER "##GGTHUNK##"
#define GG_MANIFEST_ENVAR     "GG_MANIFEST"
#define GG_THUNK_PATH_ENVAR   "__GG_THUNK_PATH__"
#define GG_ENABLED_ENVAR      "__GG_ENABLED__"
#define GG_DIR_ENVAR          "__GG_DIR__"
#define GG_VERBOSE_ENVAR      "__GG_VERBOSE__"

#define GG_INFO( ... )    do { if (__gg.verbose) fprintf(stderr, "[gg:info] " __VA_ARGS__); } while (0)
#define GG_DEBUG( ... )   do { if (__gg.verbose) fprintf(stderr, "[gg:debug] " __VA_ARGS__); } while (0)
#define GG_WARNING( ... ) do { if (__gg.verbose) fprintf(stderr, "[gg:warning] " __VA_ARGS__); } while (0)
#define GG_ERROR( ... )   do { if (__gg.verbose) fprintf(stderr, "[gg:error] " __VA_ARGS__); } while (0)

#endif
