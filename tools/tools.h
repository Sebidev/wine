/*
 * Helper functions for the Wine tools
 *
 * Copyright 2021 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_TOOLS_H
#define __WINE_TOOLS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__GNUC__) && !defined(__attribute__)
#define __attribute__(x)
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif


static inline void *xmalloc( size_t size )
{
    void *res = malloc( size ? size : 1 );

    if (res == NULL)
    {
        fprintf( stderr, "Virtual memory exhausted.\n" );
        exit(1);
    }
    return res;
}

static inline void *xrealloc (void *ptr, size_t size)
{
    void *res = realloc( ptr, size );

    if (size && res == NULL)
    {
        fprintf( stderr, "Virtual memory exhausted.\n" );
        exit(1);
    }
    return res;
}

static inline char *xstrdup( const char *str )
{
    return strcpy( xmalloc( strlen(str)+1 ), str );
}

static inline int strendswith( const char *str, const char *end )
{
    int l = strlen( str );
    int m = strlen( end );
    return l >= m && !strcmp( str + l - m, end );
}

static char *strmake( const char* fmt, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static inline char *strmake( const char* fmt, ... )
{
    int n;
    size_t size = 100;
    va_list ap;

    for (;;)
    {
        char *p = xmalloc( size );
        va_start( ap, fmt );
	n = vsnprintf( p, size, fmt, ap );
	va_end( ap );
        if (n == -1) size *= 2;
        else if ((size_t)n >= size) size = n + 1;
        else return p;
        free( p );
    }
}

/* string array functions */

struct strarray
{
    unsigned int count;  /* strings in use */
    unsigned int size;   /* total allocated size */
    const char **str;
};

static const struct strarray empty_strarray;

static inline void strarray_add( struct strarray *array, const char *str )
{
    if (array->count == array->size)
    {
	if (array->size) array->size *= 2;
        else array->size = 16;
	array->str = xrealloc( array->str, sizeof(array->str[0]) * array->size );
    }
    array->str[array->count++] = str;
}

static inline void strarray_addall( struct strarray *array, struct strarray added )
{
    unsigned int i;

    for (i = 0; i < added.count; i++) strarray_add( array, added.str[i] );
}

static inline int strarray_exists( const struct strarray *array, const char *str )
{
    unsigned int i;

    for (i = 0; i < array->count; i++) if (!strcmp( array->str[i], str )) return 1;
    return 0;
}

static inline void strarray_add_uniq( struct strarray *array, const char *str )
{
    if (!strarray_exists( array, str )) strarray_add( array, str );
}

static inline void strarray_addall_uniq( struct strarray *array, struct strarray added )
{
    unsigned int i;

    for (i = 0; i < added.count; i++) strarray_add_uniq( array, added.str[i] );
}

static inline struct strarray strarray_fromstring( const char *str, const char *delim )
{
    struct strarray array = empty_strarray;
    char *buf = xstrdup( str );
    const char *tok;

    for (tok = strtok( buf, delim ); tok; tok = strtok( NULL, delim ))
        strarray_add( &array, xstrdup( tok ));
    free( buf );
    return array;
}

static inline struct strarray strarray_frompath( const char *path )
{
    if (!path) return empty_strarray;
#if defined(_WIN32) && !defined(__CYGWIN__)
    return strarray_fromstring( path, ";" );
#else
    return strarray_fromstring( path, ":" );
#endif
}

static inline char *strarray_tostring( struct strarray array, const char *sep )
{
    char *str;
    unsigned int i, len = 1 + (array.count - 1) * strlen(sep);

    if (!array.count) return xstrdup("");
    for (i = 0; i < array.count; i++) len += strlen( array.str[i] );
    str = xmalloc( len );
    strcpy( str, array.str[0] );
    for (i = 1; i < array.count; i++)
    {
        strcat( str, sep );
        strcat( str, array.str[i] );
    }
    return str;
}

static inline void strarray_qsort( struct strarray *array, int (*func)(const char **, const char **) )
{
    if (array->count) qsort( array->str, array->count, sizeof(*array->str), (void *)func );
}

static inline const char *strarray_bsearch( const struct strarray *array, const char *str,
                                            int (*func)(const char **, const char **) )
{
    char **res = NULL;

    if (array->count) res = bsearch( &str, array->str, array->count, sizeof(*array->str), (void *)func );
    return res ? *res : NULL;
}


#endif /* __WINE_TOOLS_H */
