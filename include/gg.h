#ifndef _GG_H_
#define _GG_H_

#include <stdbool.h>

char * get_gg_file( const char * filename );
int is_dir_allowed( const char * path );

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
