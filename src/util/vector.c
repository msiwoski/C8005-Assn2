/*********************************************************************************************
Name:			vector.c

    Required:	vector.h

    Developer:  Shane Spoor

    Created On: 2017-02-17

    Description:
    This creates the vector that is used within the Servers, acts as a resizeable list.

    Revisions:
    (none)

*********************************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vector.h"

static const float VECTOR_GROWTH_RATE = 1.5;

/*********************************************************************************************
FUNCTION

    Name:		vector_init

    Prototype:	int vector_init(vector_t* vec, size_t item_size, size_t cap)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    vec - Vector 
    item_size - the item size of the vector
    cap - the cap size of the vector
	
    Description:
    Initialize the vector.

    Revisions:
	(none)

*********************************************************************************************/
int vector_init(vector_t* vec, size_t item_size, size_t cap)
{
    void* items;
    cap = cap == 0 ? VECTOR_DEFAULT_CAPACITY : cap;
    
    items = malloc(item_size * cap);
    if (!items) return -1;

    vec->items = items;
    vec->cap = cap;
    vec->size = 0;
    vec->item_size = item_size;

    return 0;
}

/*********************************************************************************************
FUNCTION

    Name:		vector_create

    Prototype:	vector_t* vector_create(size_t item_size, size_t cap)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    item_size - Vector size
    cap - The total cap size
	
    Description:
    Create the vector

    Revisions:
	(none)

*********************************************************************************************/
vector_t* vector_create(size_t item_size, size_t cap)
{
    vector_t* vec = malloc(sizeof(vector_t));
    if (!vec) return NULL;

    if (vector_init(vec, item_size, cap) == -1) return NULL;

    return vec;
}

/*********************************************************************************************
FUNCTION

    Name:		vector_remove_at

    Prototype:	void vector_remove_at(vector_t* vec, unsigned i)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    vec - the vector
    i - Position
	
    Description:
    Delete from the vector at a specific position.

    Revisions:
	(none)

*********************************************************************************************/
void vector_remove_at(vector_t* vec, unsigned i)
{
    unsigned char* items = (unsigned char*)vec->items;
    unsigned char* prev = items + (vec->item_size * i);
    unsigned char* start = prev + vec->item_size;
    unsigned char* end = items + (vec->item_size * vec->size);
    unsigned char* cur;

    /* Shift all elements past i down by one */
    for (cur = start; cur != end; cur += vec->item_size)
    {
        memcpy(prev, cur, vec->item_size);
        prev = cur;
    }

    vec->size--;
}

/*********************************************************************************************
FUNCTION

    Name:		vector_insert_at

    Prototype:	int vector_insert_at(vector_t* vec, void* item, unsigned i)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    vec - the vector
    item - temp data
    i - insert position
	
    Description:
    Insert into the vector at a position.

    Revisions:
	(none)

*********************************************************************************************/
int vector_insert_at(vector_t* vec, void* item, unsigned i)
{
    unsigned char* items;
    unsigned char* prev;
    unsigned char* start;
    unsigned char* end;
    unsigned char* cur;

    if (vec->size == vec->cap)
    {
        size_t new_cap = (size_t)roundf(vec->cap * VECTOR_GROWTH_RATE);
        if (vector_resize(vec, new_cap) == -1)
        {
            return -1;
        }
    }

    items = (unsigned char*)vec->items;
    end = items + (vec->item_size * vec->size);
    start = items + (vec->item_size * i);

    /* Shift items i through the end of the vector up by one */
    for (cur = end; cur > start; cur -= vec->item_size)
    {
        prev = cur - vec->item_size;
        memcpy(cur, prev, vec->item_size);
    }
    
    /* Copy the item into its position */
    memcpy(start, item, vec->item_size);
    vec->size++;

    return 0;
}

/*********************************************************************************************
FUNCTION

    Name:		vector_push_back

    Prototype:	int vector_push_back(vector_t* vec, void* item)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    vec - the vector
    item - temp data
	
    Description:
    Push back the vector.

    Revisions:
	(none)

*********************************************************************************************/
int vector_push_back(vector_t* vec, void* item)
{
    return vector_insert_at(vec, item, vec->size);
}

/*********************************************************************************************
FUNCTION

    Name:		vector_resize

    Prototype:	int vector_resize(vector_t* vec, size_t cap)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    vec - the vector
    tmp - temp data
	
    Description:
    Resize the vector

    Revisions:
	(none)

*********************************************************************************************/
int vector_resize(vector_t* vec, size_t cap)
{
    void* new_items = realloc(vec->items, cap * vec->item_size);
    if (!new_items) return -1;
    vec->items = new_items;
    vec->cap = cap;
    return 0;
}

/*********************************************************************************************
FUNCTION

    Name:		vector_reverse_no_alloc

    Prototype:	void vector_reverse_no_alloc(vector_t vec, void* tmp)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    vec - the vector
    tmp - temp data
	
    Description:
    Reverses the vector with no allocation.

    Revisions:
	(none)

*********************************************************************************************/
void vector_reverse_no_alloc(vector_t* vec, void* tmp)
{
    unsigned char* front;
    unsigned char* back;
    unsigned char* items = vec->items;

    for (front = items, back = items + (vec->item_size * (vec->size - 1)); front < back; front += vec->item_size, back -= vec->item_size)
    {
        memcpy(tmp, front, vec->item_size);
        memcpy(front, back, vec->item_size);
        memcpy(back, tmp, vec->item_size);
    }
}

/*********************************************************************************************
FUNCTION

    Name:		vector_reverse

    Prototype:	int vector_reverse(vector_t const* vec)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    vec - the vector
	
    Description:
    Reverses the vector

    Revisions:
	(none)

*********************************************************************************************/
int vector_reverse(vector_t* vec)
{
    unsigned char* tmp;
    unsigned char* items = (unsigned char*)vec->items;
    
    /* No space; use a temporary buffer */
    if (vec->size == vec->cap)
    {
        tmp = malloc(vec->item_size);
        if (!tmp)
        {
            return -1;
        }
        else
        {
            vector_reverse_no_alloc(vec, tmp);
            free(tmp);
            return 0;
        }
    }

    /* There's some space at the end of the vector; use that as scratch space */
    tmp = items + (vec->item_size * vec->size);
    vector_reverse_no_alloc(vec, tmp);
    return 0;
}

/*********************************************************************************************
FUNCTION

    Name:		vector_free

    Prototype:	void vector_free(vector_t const* vec)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:

    Return Values:
    vec - the vector
	
    Description:
    Frees the vector

    Revisions:
	(none)

*********************************************************************************************/
void vector_free(vector_t const* vec)
{
    free(vec->items);
}