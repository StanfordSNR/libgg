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
#include "gg.pb.h"
#include "pb_decode.h"

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

void abort_gg()
{
  abort();
}

/*******************************************************************************
 * READING THE THUNK                                                           *
 ******************************************************************************/

bool indata_decode_callback( pb_istream_t * stream,
                          const pb_field_t * field,
                          void ** arg )
{
  char buffer[ PATH_MAX ] = { 0 };

  if ( stream->bytes_left >= PATH_MAX ) {
    GG_ERROR( "large data value" );
    return false;
  }

  pb_read( stream, (void *)buffer, stream->bytes_left );
  char * eqpos = strchr( buffer, '=' );

  if ( !eqpos ) {
    /* no need to store this, there's no filename! */
    return true;
  }

  eqpos[ 0 ] = '\0';

  if ( ( strlen( buffer ) >= HASH_MAX_LENGTH ) ||
       ( strlen( __gg.dir ) + strlen( buffer ) + 1 >= PATH_MAX ) ) {
    GG_ERROR( "large hash" );
    return false;
  }

  InData indata = { 0 };

  strcpy( indata.hash, buffer );
  strcpy( indata.filename, eqpos + 1 );

  indata.gg_path[ 0 ] = '\0';
  strcat( indata.gg_path, __gg.dir );
  strcat( indata.gg_path, "/" );
  strcat( indata.gg_path, indata.hash );

  indata.size = strtoul( indata.hash + strlen( indata.hash ) - 8, NULL, 16 );
  indata.enabled = true;

  vector_InData_push_back( &__gg.indata, &indata );

  GG_INFO( "infile: %s (%.8s...)\n", indata.filename, indata.hash );

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
  const long fsize = ftell( fp );
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
    abort_gg();
  }

  result.data.funcs.decode = &indata_decode_callback;
  result.executables.funcs.decode = &indata_decode_callback;

  if( !pb_decode( &is, gg_protobuf_Thunk_fields, &result ) ) {
    GG_ERROR( "reading the thunk failed" );
    abort_gg();
  };

  free( fdata );
  GG_INFO( "thunk processed: %s\n", __gg.thunk_file );
}

/*******************************************************************************
 * READING THE MANIFEST                                                        *
 ******************************************************************************/

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

  enum { Z = 0, O1, O2, D1 } state;
  DummyDir ddir_temp;
  Output output_temp;

  while ( current < fdata_end ) {
    char * token_end = strchr( current, '\0' );
    if ( token_end == NULL ) {
      return abort_gg();
    }

    switch ( state ) {
    case Z:
    {
      switch( *current ) {
      case 'D': state = D1; break;
      case 'O': state = O1; break;
      default:
        return abort_gg();
      }

      break;
    }

    case O1:
    {
      if ( strlen( current ) >= PATH_MAX ) { return abort_gg(); }

      output_temp.filename[ 0 ] = output_temp.tag[ 0 ] = '\0';

      /* reading the file name */
      strcpy( output_temp.filename, current );

      state = O2;
      break;
    }

    case O2:
    {
      if ( strlen( current ) >= PATH_MAX ) { return abort_gg(); }

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
      if ( strlen( current ) >= PATH_MAX ) { return abort_gg(); }

      /* reading the dir name */
      ddir_temp.path[ 0 ] = '\0';

      strcpy( ddir_temp.path, current );
      vector_DummyDir_push_back( &__gg.indirs, &ddir_temp );

      GG_INFO( "indir: %s\n", ddir_temp.path );

      state = Z;
      break;
    }

    default:
      return abort_gg();
    }

    current = token_end + 1;
  }

  free( fdata );
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
