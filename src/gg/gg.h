#ifndef _GG_H_
#define _GG_H_


/*
 * This checks the ENV for a mapping from the filename to the
 * hash of the file, which is the name of the file in the .gg directory.
 * The filenames are 64 chars so the char * must be 65 chars.
 */
int get_gg_file(const char *filename, char * gg_file_out);

#endif