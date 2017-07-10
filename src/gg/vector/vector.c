#define INITIAL_SIZE 10

void vector_init( vector * v )
{
  v->data = NULL;
  v->size = 0;
  v->count = 0;
}

void vector_push_back( vector * v, void * d )
{
  if ( v->size == 0 ) {
    v->data = malloc( sizeof( void * ) * INITIAL_SIZE );
    v->size = INITIAL_SIZE;
  }

  if ( v->size == v->count ) {
    v->size *= 2;
    v->data = realloc( v->data, sizeof( void * ) * v->size );
  }

  v->data[ v->count++ ] = d;
}

void * vector_at( vector * v, size_t index ) {
  if ( index >= v->count ) {
    return NULL;
  }

  return v->data[ index ];
}

void vector_free( vector * v ) {
  free( v->data );
}
