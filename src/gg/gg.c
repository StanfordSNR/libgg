#include "stdio_impl.h"
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include "syscall.h"
#include "libc.h"

#include "gg.h"

VECTORFUNCS( InData );
VECTORFUNCS( DummyDir );
VECTORFUNCS( AllowedFile );
VECTORFUNCS( Output );

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

void abort_manifest_read()
{
  abort();
}

void __gg_read_manifest()
{
  /* read the thunk file into a buffer */
  FILE * fp = fdopen( unrestricted_open( __gg.manifest_file, O_RDONLY ), "r" );

  if ( fp == NULL ) {
    GG_ERROR( "cannot open file: %s\n", __gg.manifest_file );
    return;
  }

  fseek( fp, 0, SEEK_END );
  const long fsize = ftell( fp );
  fseek( fp, 0, SEEK_SET );

  char * const fdata = malloc( fsize );
  fread( fdata, fsize, 1, fp );
  fclose( fp );

  size_t index = 0;
  char * const fdata_end = fdata + fsize;
  const char * current = fdata;

  enum { Z = 0, F1, F2, O1, O2, D1 } state;
  InData indata_temp;
  DummyDir ddir_temp;
  Output output_temp;

  while ( current < fdata_end ) {
    char * token_end = strchr( current, '\0' );
    if ( token_end == NULL ) {
      return abort_manifest_read();
    }

    switch ( state ) {
    case Z:
    {
      switch( *current ) {
      case 'F': state = F1; break;
      case 'D': state = D1; break;
      case 'O': state = O1; break;
      default:
        return abort_manifest_read();
      }

      break;
    }

    case F1:
    {
      /* resetting indata_temp */
      indata_temp.hash[ 0 ] = indata_temp.gg_path[ 0 ]
                            = indata_temp.filename[ 0 ] = '\0';

      /* reading the filename */
      if ( strlen( current ) >= PATH_MAX ) { return abort_manifest_read(); }

      strcpy( indata_temp.filename, current );

      state = F2;
      break;
    }

    case F2:
    {
      /* reading the hash */
      if ( strlen( current ) > 64 ) { return abort_manifest_read(); }

      strcpy( indata_temp.hash, current );

      if ( strlen( __gg.dir ) + strlen( indata_temp.hash ) + 1 >= PATH_MAX ) {
        return abort_manifest_read();
      }

      strcat( indata_temp.gg_path, __gg.dir );
      strcat( indata_temp.gg_path, "/" );
      strcat( indata_temp.gg_path, indata_temp.hash );

      indata_temp.size = strtoul( indata_temp.hash + strlen( indata_temp.hash ) - 8,
                                  NULL, 16 );

      indata_temp.enabled = true;
      vector_InData_push_back( &__gg.indata, &indata_temp );

      GG_INFO( "data: %s (%.8s...)\n", indata_temp.filename, indata_temp.hash );

      state = Z;
      break;
    }

    case O1:
    {
      if ( strlen( current ) >= PATH_MAX ) { return abort_manifest_read(); }

      output_temp.filename[ 0 ] = output_temp.tag[ 0 ] = '\0';

      /* reading the file name */
      strcpy( output_temp.filename, current );

      state = O2;
      break;
    }

    case O2:
    {
      if ( strlen( current ) >= PATH_MAX ) { return abort_manifest_read(); }

      output_temp.created = false;

      /* reading the tag */
      strcpy( output_temp.tag, current );

      vector_Output_push_back( &__gg.outputs, &output_temp );

      GG_INFO( "output: %s (%s)\n", output_temp.filename, output_temp.tag );

      state = Z;
      break;
    }

    case D1:
    {
      if ( strlen( current ) >= PATH_MAX ) { return abort_manifest_read(); }

      /* reading the dir name */
      ddir_temp.path[ 0 ] = '\0';

      strcpy( ddir_temp.path, current );
      vector_DummyDir_push_back( &__gg.indirs, &ddir_temp );

      GG_INFO( "indir: %s\n", ddir_temp.path );

      state = Z;
      break;
    }

    default:
      return abort_manifest_read();
    }

    current = token_end + 1;
  }

  GG_INFO( "manifest processed: %s\n", __gg.manifest_file );
}

int get_indata_index( const char * filename )
{
  for ( size_t i = 0; i < __gg.indata.count; i++ ) {
    if ( __gg.indata.data[ i ].enabled &&
         strcmp( filename, __gg.indata.data[ i ].filename ) == 0 ) {
      return i;
    }
  }

  return -1;
}

char * __gg_get_filename( const char * filename )
{
  int index = get_indata_index( filename );
  return ( index == -1 ) ? NULL : __gg.indata.data[ index ].gg_path;
}

int __gg_stat( const char * org_filename, struct stat * restrict buf )
{
  int file_index = 0;
  int retval = -1;
  bool is_directory = false;
  bool is_allowed_file = false;
  off_t size = 0;

  char * filename = __gg_normalize_path( org_filename, NULL );

  GG_DEBUG( "gg_stat(path=\"%s\", npath=\"%s\") = ", org_filename, filename );

  /* let's see if this file is in infiles. */
  for ( size_t i = 0; i < __gg.indata.count; i++, file_index++ ) {
    if ( __gg.indata.data[ i ].enabled &&
         strcmp( filename, __gg.indata.data[ i ].filename ) == 0 ) {
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
      if ( strcmp( org_filename, __gg.allowed_files.data[ i ].path ) == 0 ) {
        retval = 0;
        is_directory = false;
        is_allowed_file = true;
        break;
      }
    }
  }

  if ( retval != 0 ) {
    /* let's see if this an output */
    for ( size_t i = 0; i < __gg.outputs.count; i++ , file_index++ ) {
      if ( __gg.outputs.data[ i ].created &&
           ( strcmp( __gg.outputs.data[ i ].filename, filename ) == 0 ) ) {
        retval = 0;
        file_index = -1;
      }
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
    size = ( is_directory ? 0 : __gg.indata.data[ file_index ].size );
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

bool __gg_is_allowed( const char * filename, const bool check_indata )
{
  if ( check_indata ) {
    if ( get_indata_index( filename ) != -1 ) {
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
  int index = get_indata_index( filename );

  if ( index != - 1 ) {
    __gg.indata.data[ index ].enabled = false;
  }
}

Output * __gg_get_output( const char * filename )
{
  for ( size_t i = 0; i < __gg.outputs.count; i++ ) {
    if ( strcmp( __gg.outputs.data[ i ].filename, filename ) == 0 ) {
      return &__gg.outputs.data[ i ];
    }
  }

  return NULL;
}
