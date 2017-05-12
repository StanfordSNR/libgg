#ifndef _GG_H_
#define _GG_H_


/*
 * This checks the ENV for a mapping from the filename to the
 * hash of the file, which is the name of the file in the .gg directory.
 */
char *get_gg_file( const char *filename );

#define GG_ENV_VAR  "GG"

#endif
