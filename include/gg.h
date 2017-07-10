#ifndef _GG_H_
#define _GG_H_

/* Please note that the user is responsible to free the memory for the resulting
   string */
char * get_gg_file( const char * filename );

#define GG_THUNK_MAGIC_NUMBER "##GGTHUNK##"

#define GG_THUNK_PATH_ENVAR "__GG_THUNK_PATH__"
#define GG_ENABLED_ENVAR    "__GG_ENABLED__"
#define GG_DIR_ENVAR        "__GG_DIR__"
#define GG_VERBOSE_ENVAR    "__GG_VERBOSE__"

#endif
