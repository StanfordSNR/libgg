#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "gg.h"

int main(){
    char *filename = "foo.c";
    char *env_name = "foo_c";
    char *hash = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
    setenv( env_name, hash, 0 );

    char *ret = get_gg_file( filename );
    assert( ret != NULL );
    assert( strcmp( ret, hash ) == 0 );
}
