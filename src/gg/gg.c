#include "stdio_impl.h"
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "syscall.h"
#include "libc.h"

#include "util/vector.h"

#include "gg.h"
#include "gg.pb.h"
#include "pb_decode.h"

/* from fcntl/open.c */
int unrestricted_open(const char *filename, int flags, ...)
{
  mode_t mode = 0;

  if ((flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE) {
  va_list ap;
  va_start(ap, flags);
  mode = va_arg(ap, mode_t);
  va_end(ap);
  }

  int fd = __sys_open_cp(filename, flags, mode);
  if (fd>=0 && (flags & O_CLOEXEC))
  __syscall(SYS_fcntl, fd, F_SETFD, FD_CLOEXEC);

  return __syscall_ret(fd);
}

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

VECTOR( InFile );
VECTOR( InDir );

static vector_InFile infiles;
static vector_InDir indirs;

static bool thunk_read = false;

bool str_decode_callback( pb_istream_t * stream,
                                      const pb_field_t * field,
                                      void ** arg )
{
  pb_read( stream, *arg, stream->bytes_left );
  return true;
}

bool infile_decode_callback( pb_istream_t * stream,
                             const pb_field_t * field,
                             void ** arg )
{
  gg_protobuf_InFile infile_proto = {};

  InFile infile = { 0 };

  infile_proto.filename.funcs.decode = &str_decode_callback;
  infile_proto.filename.arg = infile.filename;
  infile_proto.hash.funcs.decode = &str_decode_callback;
  infile_proto.hash.arg = infile.hash;

  if ( !pb_decode( stream, gg_protobuf_InFile_fields, &infile_proto ) ) {
    return false;
  };

  static const char * gg_dir = NULL;

  if ( gg_dir == NULL ) {
    gg_dir = getenv( GG_DIR_ENVAR );
    if ( gg_dir == NULL ) {
      fprintf( stderr, "[gg] gg directory is not set, using default (.gg).\n" );
      gg_dir = ".gg";
    }
  }

  if ( strnlen( infile.hash, 1 ) ) {
    if ( strlen( gg_dir ) + strlen( infile.hash ) + 1 >= PATH_MAX ) {
      fprintf( stderr, "[gg] gg path is longer than PATH_MAX, aborted." );
      return false;
    }

    infile.gg_path[ 0 ] = '\0';
    strcat( infile.gg_path, gg_dir );
    strcat( infile.gg_path, "/" );
    strcat( infile.gg_path, infile.hash );

    infile.order = infile_proto.order;
    vector_InFile_push_back( &infiles, &infile );
  }
  else {
    /* this is not an infile, it's an indirectory */
    InDir indir = { 0 };
    strncpy( indir.path, infile.filename, PATH_MAX );
    vector_InDir_push_back( &indirs, &indir );
  }

  return true;
}

gg_protobuf_Thunk read_thunk()
{
  char * thunk_filename = getenv( GG_THUNK_PATH_ENVAR );
  gg_protobuf_Thunk result = {};

  if ( thunk_filename == NULL ) {
    fprintf( stderr, "[gg] Cannot find thunk filename.\n" );
    return result;
  }

  /* read the thunk file into a buffer */
  FILE * fp = fdopen( unrestricted_open( thunk_filename, O_RDONLY ), "r" );

  if ( fp == NULL ) {
    fprintf( stderr, "[gg] Cannot open file: %s\n", thunk_filename );
    return result;
  }

  fseek( fp, 0, SEEK_END );
  long fsize = ftell( fp );
  fseek( fp, 0, SEEK_SET );

  uint8_t * fdata = malloc( fsize );
  fread( fdata, fsize, 1, fp );
  fclose( fp );

  /* create istream for pb */
  pb_istream_t is = pb_istream_from_buffer( fdata, fsize );

  uint8_t magic_num[ sizeof( GG_THUNK_MAGIC_NUMBER ) ];
  pb_read( &is, magic_num, sizeof( GG_THUNK_MAGIC_NUMBER ) - 1 );
  magic_num[ sizeof( GG_THUNK_MAGIC_NUMBER ) - 1 ] = '\0';

  if ( strcmp( GG_THUNK_MAGIC_NUMBER, ( char * )magic_num ) != 0 ) {
    fprintf( stderr, "[gg] Not a thunk: %s\n", thunk_filename );
    return result;
  }

  result.infiles.funcs.decode = &infile_decode_callback;
  pb_decode( &is, gg_protobuf_Thunk_fields, &result );

  return result;
}

char * get_gg_file( const char * filename )
{
  if ( !thunk_read ) {
    read_thunk();
    thunk_read = true;
  }

  for ( size_t i = 0; i < infiles.count; i++ ) {
    if ( strcmp( filename, infiles.data[ i ].filename ) == 0 ) {
      return infiles.data[ i ].gg_path;
    }
  }

  return NULL;
}

bool is_dir_allowed( const char * path )
{
  /* XXX this is not thread-safe */
  if ( !thunk_read ) {
    read_thunk();
    thunk_read = true;
  }

  for ( size_t i = 0; i < indirs.count; i++ ) {
    if ( strcmp( path, indirs.data[ i ].path ) == 0 ) {
      return true;
    }
  }

  return false;
}
