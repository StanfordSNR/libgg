#ifndef GG_VECTOR_H
#define GG_VECTOR_H

#define INITIAL_SIZE 10

#define VECTORDEF(x) typedef struct \
{ \
  size_t size; \
  size_t count; \
  x * data; \
} vector_##x; \
\
void vector_##x##_init( vector_##x * v ); \
x * vector_##x##_push_back( vector_##x * v, x * d ); \
x * vector_##x##_at( vector_##x * v, size_t index ); \
void vector_##x##_free( vector_##x * v );

#define VECTORFUNCS(x) \
void vector_##x##_init( vector_##x * v ) \
{ \
  v->data = NULL; \
  v->size = 0; \
  v->count = 0; \
} \
\
x * vector_##x##_push_back( vector_##x * v, x * d ) \
{ \
  if ( v->size == 0 ) { \
    v->size = INITIAL_SIZE; \
    v->data = malloc( sizeof( x ) * INITIAL_SIZE ); \
  } \
\
  if ( v->size == v->count ) { \
    v->size *= 2; \
    v->data = realloc( v->data, sizeof( x ) * v->size ); \
  } \
\
  v->data[ v->count++ ] = *d; \
  return &v->data[ v->count - 1 ]; \
} \
\
x * vector_##x##_at( vector_##x * v, size_t index ) { \
  if ( index >= v->count ) { \
    return NULL; \
  } \
\
  return &v->data[ index ]; \
} \
\
void vector_##x##_free( vector_##x * v ) { \
  free( v->data ); \
}

#endif /* GG_VECTOR_H */
