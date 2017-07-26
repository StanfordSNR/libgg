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

#include "gg.h"
#include "gg.pb.h"
#include "pb_decode.h"

extern char * normalize_path( const char * pathname, char * base );

VECTORFUNCS( InFile );
VECTORFUNCS( InDir );
VECTORFUNCS( AllowedFile );

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

  if ( strnlen( infile.hash, 1 ) ) {
    if ( strlen( __gg.dir ) + strlen( infile.hash ) + 1 >= PATH_MAX ) {
      GG_ERROR( "gg path is longer than PATH_MAX, aborted." );
      return false;
    }

    infile.gg_path[ 0 ] = '\0';
    strcat( infile.gg_path, __gg.dir );
    strcat( infile.gg_path, "/" );
    strcat( infile.gg_path, infile.hash );

    infile.order = infile_proto.order;
    infile.size = infile_proto.size;
    infile.enabled = true;
    vector_InFile_push_back( &__gg.infiles, &infile );

    GG_INFO( "infile: %s (%.8s...)\n", infile.filename, infile.hash );
  }
  else {
    /* this is not an infile, it's an indirectory */
    InDir indir = { 0 };
    strncpy( indir.path, infile.filename, PATH_MAX );
    vector_InDir_push_back( &__gg.indirs, &indir );

    GG_INFO( "indir: %s\n", indir.path );
  }

  return true;
}

void __gg_read_thunk()
{
  gg_protobuf_Thunk result = {};

  /* read the thunk file into a buffer */
  FILE * fp = fdopen( unrestricted_open( __gg.thunk_file, O_RDONLY ), "r" );

  if ( fp == NULL ) {
    GG_ERROR( "cannot open file: %s\n", __gg.thunk_file );
    return;
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
    GG_ERROR( "not a thunk: %s\n", __gg.thunk_file );
    return;
  }

  result.infiles.funcs.decode = &infile_decode_callback;
  result.outfile.funcs.decode = &str_decode_callback;
  result.outfile.arg = __gg.outfile;
  pb_decode( &is, gg_protobuf_Thunk_fields, &result );

  GG_INFO( "thunk processed: %s\n", __gg.thunk_file );
  GG_INFO( "outfile: %s\n", __gg.outfile );
}

int get_infile_index( const char * filename )
{
  for ( size_t i = 0; i < __gg.infiles.count; i++ ) {
    if ( __gg.infiles.data[ i ].enabled &&
         strcmp( filename, __gg.infiles.data[ i ].filename ) == 0 ) {
      return i;
    }
  }

  return -1;
}

char * __gg_get_filename( const char * filename )
{
  int index = get_infile_index( filename );
  return ( index == -1 ) ? NULL : __gg.infiles.data[ index ].gg_path;
}

int __gg_stat( const char * org_filename, struct stat * restrict buf )
{
  int file_index = 0;
  int retval = -1;
  bool is_directory = false;
  bool is_allowed_file = false;
  off_t size = 0;

  char * filename = normalize_path( filename, NULL );

  GG_DEBUG( "gg_stat(path=\"%s\", npath=\"%s\") = ", org_filename, filename );

  /* let's see if this file is in infiles. */
  for ( size_t i = 0; i < __gg.infiles.count; i++, file_index++ ) {
    if ( __gg.infiles.data[ i ].enabled &&
         strcmp( filename, __gg.infiles.data[ i ].filename ) == 0 ) {
      retval = 0; // we found something!
      is_directory = false;
      break;
    }
  }

  if ( retval != 0 ) {
    for ( size_t i = 0; i < __gg.indirs.count; i++, file_index++ ) {
      if ( strcmp( filename, __gg.indirs.data[ i ].path ) == 0 ) {
        retval = 0;
        is_directory = true;
        break;
      }
    }
  }

  if ( retval != 0 ) {
    /* let's see if this file is in allowed files */
    for ( size_t i = 0; i < __gg.allowed_files.count; i++, file_index++ ) {
      if ( strcmp( filename, __gg.allowed_files.data[ i ].path ) == 0 ) {
        retval = 0;
        is_directory = false;
        is_allowed_file = true;
        break;
      }
    }
  }

  if ( retval != 0 ) {
    if ( __gg.outfile_created && ( strcmp( __gg.outfile, filename ) == 0 ) ) {
      retval = 0;
      file_index = -1;
    }
  }

  if ( retval != 0 ) {
    GG_DEBUG( "-1 (ENOENT)\n" );
    
    errno = ENOENT;
    free( filename );
    return -1;
  }

  if ( is_allowed_file || file_index == -1 ) {
    /* this might cause some problems... */
    size = 2048;
  }
  else {
    size = ( is_directory ? 0 : __gg.infiles.data[ file_index ].size );
  }

  buf->st_dev = 1; /* ID of device containing file */
  buf->st_ino = 42 + ( file_index );/* inode number */
  buf->st_mode = ( is_directory ? 0040755 : 0100755 ); /* protection */
  buf->st_nlink = 2; /* number of hard links */
  buf->st_uid = 1000; /* user ID of owner */
  buf->st_gid = 1000; /* group ID of owner */
  buf->st_size = size; /* total size, in bytes */
  buf->st_blksize = 4096; /* blocksize for file system I/O */
  buf->st_blocks = ( buf->st_size / 512 ); /* number of 512B blocks allocated */
  buf->st_atime = 231508800; /* time of last access */
  buf->st_mtime = 231508800; /* time of last modification */
  buf->st_ctime = 231508800; /* time of last status change */

  GG_DEBUG( "0\n" );

  free( filename );
  return 0;
}

bool __gg_is_allowed( const char * filename, const bool check_infiles )
{
  if ( check_infiles ) {
    if ( get_infile_index( filename ) != -1 ) {
      return true;
    }
  }

  for (size_t i = 0; i < __gg.allowed_files.count; i++) {
    if (strcmp(filename, __gg.allowed_files.data[ i ].path) == 0) {
      return true;
    }
  }

  return false;
}

void __gg_disable_infile( const char * filename )
{
  int index = get_infile_index( filename );

  if ( index != - 1 ) {
    __gg.infiles.data[ index ].enabled = false;
  }
}
