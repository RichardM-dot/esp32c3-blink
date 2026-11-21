/*
 * libc_stubs.c — the few C library routines the kernel core needs.
 *
 * We link with -nostdlib, so these must be provided locally.  Compiled with
 * -fno-builtin so the compiler does not turn the loops back into calls.
 */
#include <stddef.h>

void * memcpy( void * dst, const void * src, size_t n )
{
    unsigned char * d = ( unsigned char * ) dst;
    const unsigned char * s = ( const unsigned char * ) src;

    while( n-- )
    {
        *d++ = *s++;
    }
    return dst;
}

void * memset( void * dst, int c, size_t n )
{
    unsigned char * d = ( unsigned char * ) dst;

    while( n-- )
    {
        *d++ = ( unsigned char ) c;
    }
    return dst;
}

void * memmove( void * dst, const void * src, size_t n )
{
    unsigned char * d = ( unsigned char * ) dst;
    const unsigned char * s = ( const unsigned char * ) src;

    if( d < s )
    {
        while( n-- )
        {
            *d++ = *s++;
        }
    }
    else
    {
        d += n;
        s += n;
        while( n-- )
        {
            *--d = *--s;
        }
    }
    return dst;
}

int memcmp( const void * a, const void * b, size_t n )
{
    const unsigned char * pa = ( const unsigned char * ) a;
    const unsigned char * pb = ( const unsigned char * ) b;

    while( n-- )
    {
        if( *pa != *pb )
        {
            return ( int ) *pa - ( int ) *pb;
        }
        pa++;
        pb++;
    }
    return 0;
}

size_t strlen( const char * s )
{
    const char * p = s;

    while( *p )
    {
        p++;
    }
    return ( size_t ) ( p - s );
}

char * strcpy( char * d, const char * s )
{
    char * r = d;

    while( ( *d++ = *s++ ) != '\0' )
    {
    }
    return r;
}

void abort( void )
{
    for( ;; )
    {
        __asm__ volatile( "nop" );
    }
}
