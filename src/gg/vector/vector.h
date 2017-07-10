#ifndef GG_VECTOR_H
#define GG_VECTOR_H

typedef struct
{
  size_t size;
  size_t count;
  void ** data;
} vector;

void vector_init( vector * v );
void vector_push_back( vector * v, void * d );
void * vector_at( vector * v, size_t index );
void vector_free( vector * v );

#endif /* GG_VECTOR_H */
