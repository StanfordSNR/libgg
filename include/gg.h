#ifndef _GG_H_
#define _GG_H_

/*
 * This checks the ENV for a mapping from the filename to the
 * hash of the file, which is the name of the file in the .gg directory.
 */

char * get_gg_file( const char * filename );

#define GG_THUNK_MAGIC_NUMBER "##GGTHUNK##"

#define GG_THUNK_PATH_ENVAR "__GG_THUNK_PATH__"
#define GG_ENABLED_ENVAR    "__GG_ENABLED__"
#define GG_DIR_ENVAR        "__GG_DIR__"
#define GG_VERBOSE_ENVAR    "__GG_VERBOSE__"

#endif
