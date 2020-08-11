/*
 * implicit free lists with boundary tag optimization first
 * If the test result is not satisfied,we will consider other implimentation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};


//define the size of the header
#define HEADERSIZE 4

//define the size of the word
#define WORDSIZE 4

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) ((((size) + (ALIGNMENT-1)) & ~0x7)/WORDSIZE)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//define the initial heapsize
#define INITIALHEAP 1024

//define some global variables
//points to the first block of the heap
uint32_t* header=NULL;
//points to the last block of the heap
uint32_t* tailer=NULL;
//the next heap size that is going to allocate
uint32_t allocate_size=16;

//all function definitions
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);

//self-defined auxillary functions

//set the begin block(auxillary function for the mm_init function),to mark the begin of the list
void set_begin_block(uint32_t* ptr);
//set the end block(auxillary function for the mm_init function),to mark the end of the list
void set_end_block(uint32_t* ptr);

//ptr: the first word of the free block,size:the number of words in the block
void write_free_block(uint32_t* ptr,int word_size);
//ptr: the header or the tailer of the block
uint32_t get_block_size(uint32_t* ptr);
//is the block has been allocated(ptr is the header or the tailer of the block)
int is_block_allocated(uint32_t* ptr);
//ptr : the header of the block
uint32_t* get_tailer_from_header(uint32_t* ptr);
//ptr : the tailer of the block
uint32_t* get_header_from_tailer(uint32_t* ptr);
//get next block,ptr is the header of the block,return null if it is the last block
uint32_t* get_next_block(uint32_t* ptr);
//return the header of the previous block,ptr is the header of the block,return null if it is the first block
uint32_t* get_prev_block(uint32_t* ptr);
//expand the heap when the allocated memory has run out and return the header of the free block
uint32_t* expand_heap();
//find the available free block in the list by the first fit strategy(assume that the word_size has been processed)
uint32_t* find_free_block_by_first_fit(int word_size);
//find the available free block in the list by the best fit strategy(assume that the word_size has been processed)
uint32_t* find_free_block_by_best_fit(int word_size);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int initial_free_word_payload=ALIGN(allocate_size);
    int initial_word_number=initial_free_word_payload+6;
    header=mem_sbrk(initial_word_number*WORDSIZE);
    if(header==-1)
        return -1;
    
    //mark the begining of the list
    header+=1;
    set_begin_block(header);

    //set the header to the actual header of the list
    header+=2;
    write_free_block(header,initial_free_word_payload+2);

    tailer+=5+initial_free_word_payload;
    set_end_block(tailer);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

int is_block_allocated(uint32_t* ptr)
{
    return (*ptr)&0x1;
}

void set_begin_block(uint32_t* ptr)
{
    //first write the first two word that mark the beginning of the list
    *ptr=8;
    *ptr=(*ptr)|0x1;
    ptr++;
    *ptr=8;
    *ptr=(*ptr)|0x1;
}


void set_end_block(uint32_t* ptr)
{
    *ptr=0;
    *ptr=(*ptr)|0x1;
}

void write_free_block(uint32_t* ptr,int word_size)
{
    *ptr=word_size*WORDSIZE;
    *(ptr+(word_size-1))=word_size*WORDSIZE;
}

uint32_t get_block_size(uint32_t* ptr)
{
    return ((*ptr)&(~0x7))/WORDSIZE;
}


uint32_t* get_tailer_from_header(uint32_t* ptr)
{
    int word_size=get_block_size(ptr);
    return ptr+(word_size-1);
}

uint32_t* get_header_from_tailer(uint32_t* ptr)
{
    int word_size=get_block_size(ptr);
    return ptr-((word_size-1));
}

uint32_t* get_next_block(uint32_t* ptr)
{
    if(ptr==tailer)
        return NULL;
    int word_size=get_block_size(ptr);
    return ptr+word_size;
}

uint32_t* get_prev_block(uint32_t* ptr)
{
    if(ptr==header)
        return NULL;
    return get_header_from_tailer(ptr-1);
}

uint32_t* expand_heap(uint32_t need_bytes)
{
    int payload_word_size=ALIGN(need_bytes);
    void* ptr=mem_sbrk((payload_word_size+2)*WORDSIZE);

    write_free_block(tailer,payload_word_size+2);
    set_end_block(tailer+payload_word_size+2);
    tailer+=(payload_word_size+2);
    return ((uint32_t*)ptr)-1;
}

uint32_t* find_free_block_by_first_fit(int word_size)
{
    uint32_t* ptr=header;
    uint32_t block_size;
    while(1)
    {
        if(ptr==tailer)
            return NULL;
        else if(!is_block_allocated(ptr))
        {
            block_size=get_block_size(ptr);
            if(block_size>=word_size)
                return ptr;
        }
        else
        {
            ptr=get_next_block(ptr);
        }
    }
}

uint32_t* find_free_block_by_best_fit(int word_size)
{
    
}