/*
 * Copyright (c) 2007 Ivan Leben
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
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/*------------------------------------------------------------
 * The base for any type of array. According to appropriate
 * macro definitions, specific array types will be generated
 * and their manipulation functions respectively.
 *
 * This code assumes the following are defined:
 * _ITEM_T: the type of the items in the array
 * _ARRAY_T: the name of the structure
 * _FUNC_T: the prefix to prepend to each function
 *
 * And at least one of these:
 * _ARRAY_DECLARE: generate structure declaration
 * _ARRAY_DEFINE: generate function definitions
 *-----------------------------------------------------------*/

#ifndef __SHARRAYBASE_H
#define __SHARRAYBASE_H

#include "shDefs.h"

#define VAL(x,y) x ## y
#define JN(x,y) VAL(x,y)

#endif

/*--------------------------------------------
 * The rest is not #ifndef protected to allow
 * for various array type definitions.
 *--------------------------------------------*/


#ifdef _ARRAY_DECLARE
typedef struct
{
  _ITEM_T *items;
  SHint32 capacity;
  SHint32 size;
  SHint outofmemory;
  
} _ARRAY_T;
#endif


void JN(_ARRAY_T,_ctor) (_ARRAY_T *a)
#ifdef _ARRAY_DEFINE
{ 
  a->items = (_ITEM_T*)malloc(sizeof(_ITEM_T));
  
  if (!a->items) {
    a->outofmemory = 1;
    a->capacity = 0;
    a->size = 0;
    return;
  }
  
  a->outofmemory = 0;
  a->capacity = 1;
  a->size = 0;
}
#else
;
#endif


void JN(_ARRAY_T,_dtor) (_ARRAY_T *a)
#ifdef _ARRAY_DEFINE
{
  if (a->items) {
    free(a->items);
    a->items = NULL;
  }
}
#else
;
#endif

void JN(_FUNC_T,Clear) (_ARRAY_T *a)
#ifdef _ARRAY_DEFINE
{
  a->size = 0;
}
#else
;
#endif


/*--------------------------------------------------------
 * Set the capacity of the array. In case of reallocation
 * the items are not preserved.
 *--------------------------------------------------------*/

int JN(_FUNC_T,Realloc) (_ARRAY_T *a, SHint newsize)
#ifdef _ARRAY_DEFINE
{
  _ITEM_T *newitems = 0;
  
  SH_ASSERT(newsize > 0);
  if (newsize == a->capacity)
    return 1;
  
  newitems = (_ITEM_T*)malloc(newsize * sizeof(_ITEM_T));
  
  if (!newitems) {
    a->outofmemory = 1;
    return 0;
  }
  
  if (a->items)
    free(a->items);
  
  a->outofmemory = 0;
  a->items = newitems;
  a->capacity = newsize;
  a->size = 0;
  return 1;
}
#else
;
#endif


/*------------------------------------------------------
 * Asserts the capacity is at least [newsize]. In case
 * of reallocation items are not preserved.
 *------------------------------------------------------*/

int JN(_FUNC_T,Reserve) (_ARRAY_T *a, SHint newsize)
#ifdef _ARRAY_DEFINE
{
  _ITEM_T *newitems = 0;
  
  SH_ASSERT(newsize >= 0);
  if (newsize <= a->capacity)
    return 1;
  
  newitems = (_ITEM_T*)malloc(newsize * sizeof(_ITEM_T));
  
  if (!newitems) {
    a->outofmemory = 1;
    return 0;
  }
  
  if (a->items)
    free(a->items);
  
  a->outofmemory = 0;
  a->items = newitems;
  a->capacity = newsize;
  a->size = 0;
  return 1;
}
#else
;
#endif

/*------------------------------------------------------
 * Asserts the capacity is at least [newsize]. In case
 * of reallocation items are copied.
 *------------------------------------------------------*/

int JN(_FUNC_T,ReserveAndCopy) (_ARRAY_T *a, SHint newsize)
#ifdef _ARRAY_DEFINE
{
  _ITEM_T *newitems = 0;
  
  SH_ASSERT(newsize >= 0);
  if (newsize <= a->capacity)
    return 1;
  
  newitems = (_ITEM_T*)realloc(a->items, newsize * sizeof(_ITEM_T));
  
  if (!newitems) {
    a->outofmemory = 1;
    return 0;
  }
  
  a->outofmemory = 0;
  a->items = newitems;
  a->capacity = newsize;
  return 1;
}
#else
;
#endif


int JN(_FUNC_T,PushBack) (_ARRAY_T *a, _ITEM_T item)
#ifdef _ARRAY_DEFINE
{
  if (a->capacity == 0) {
    JN(_FUNC_T,Realloc)(a, 1);
    if (a->outofmemory)
      return 0;
  }
  
  if (a->size + 1 > a->capacity)
    JN(_FUNC_T,ReserveAndCopy)(a, a->capacity*2);
  
  if (a->outofmemory)
    return 0;
  
  a->items[a->size++] = item;
  return 1;
}
#else
;
#endif


int JN(_FUNC_T,PushBackP) (_ARRAY_T *a, _ITEM_T *item)
#ifdef _ARRAY_DEFINE
{
  if (a->capacity == 0) {
    JN(_FUNC_T,Realloc)(a, 1);
    if (a->outofmemory)
      return 0;
  }
  
  if (a->size + 1 > a->capacity)
    JN(_FUNC_T,ReserveAndCopy)(a, a->capacity*2);
  
  if (a->outofmemory)
    return 0;
  
  a->items[a->size++] = *item;
  return 1;
}
#else
;
#endif


void JN(_FUNC_T,PopBack) (_ARRAY_T *a)
#ifdef _ARRAY_DEFINE
{
  SH_ASSERT(a->size);
  --a->size;
}
#else
;
#endif


_ITEM_T JN(_FUNC_T,Front) (_ARRAY_T *a)
#ifdef _ARRAY_DEFINE
{
  SH_ASSERT(a->size);
  return a->items[0];
}
#else
;
#endif


_ITEM_T* JN(_FUNC_T,FrontP) (_ARRAY_T *a)
#ifdef _ARRAY_DEFINE
{
  SH_ASSERT(a->size);
  return &a->items[0];
}
#else
;
#endif


_ITEM_T JN(_FUNC_T,Back) (_ARRAY_T *a)
#ifdef _ARRAY_DEFINE
{
  SH_ASSERT(a->size);
  return a->items[a->size - 1];
}
#else
;
#endif


_ITEM_T* JN(_FUNC_T,BackP) (_ARRAY_T *a)
#ifdef _ARRAY_DEFINE
{
  SH_ASSERT(a->size);
  return &a->items[a->size - 1];
}
#else
;
#endif


_ITEM_T JN(_FUNC_T,At) (_ARRAY_T *a, SHint index)
#ifdef _ARRAY_DEFINE
{
  SH_ASSERT(index >= 0);
  SH_ASSERT(index < a->size);
  return a->items[index];
}
#else
;
#endif


_ITEM_T* JN(_FUNC_T,AtP) (_ARRAY_T *a, SHint index)
#ifdef _ARRAY_DEFINE
{
  SH_ASSERT(index >= 0);
  SH_ASSERT(index < a->size);
  return &a->items[index];
}
#else
;
#endif

SHint JN(_FUNC_T,Find) (_ARRAY_T *a, _ITEM_T item)
#ifdef _ARRAY_DEFINE
{
  int i;
  for (i=0; i<a->size; ++i) {
    #ifdef _COMPARE_T
    if (_COMPARE_T(a->items[i], item))
      return i;
    #else
    if (a->items[i] == item)
      return i;
    #endif
  }
  
  return -1;
}
#else
;
#endif

void JN(_FUNC_T,RemoveAt) (_ARRAY_T *a, SHint index)
#ifdef _ARRAY_DEFINE
{
  int i;
  SH_ASSERT(index >= 0);
  SH_ASSERT(index < a->size);
  for (i=index; i<a->size-1; ++i)
    a->items[i] = a->items[i+1];
  a->size--;
}
#else
;
#endif

#undef _ITEM_T
#undef _ARRAY_T
#undef _FUNC_T
#undef _COMPARE_T
#undef _ARRAY_DEFINE
#undef _ARRAY_DECLARE
