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
#include <math.h>

extern int free_index;
extern int malloc_index;
extern int realloc_index;

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
    ""};

//define the size of the header
#define HEADERSIZE 4

//define the size of the word
#define WORDSIZE 4

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) ((((size) + (ALIGNMENT - 1)) & ~0x7) / WORDSIZE)

#define MINBLOCKSIZE 4

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//define the initial heapsize
#define INITIALHEAP 1024

//define some global variables
//points to the first block of the heap
uint32_t *header = NULL;
//points to the last block of the heap
uint32_t *tailer = NULL;

#define N 32
uint32_t *segregated_list[N];

//all function definitions
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);

//self-defined auxillary functions

//set the begin block(auxillary function for the mm_init function),to mark the begin of the list
void set_begin_block(uint32_t *ptr);
//set the end block(auxillary function for the mm_init function),to mark the end of the list
void set_end_block(uint32_t *ptr);
//ptr: the header or the tailer of the block
uint32_t get_block_size(uint32_t *ptr);
//is the block has been allocated(ptr is the header or the tailer of the block)
int is_block_allocated(uint32_t *ptr);
//ptr : the header of the block
uint32_t *get_tailer_from_header(uint32_t *ptr);
//ptr : the tailer of the block
uint32_t *get_header_from_tailer(uint32_t *ptr);
//get the next block in the list,return null if it is the last block
uint32_t *get_next_block(uint32_t *ptr);
//get the previous block in the list,return null if it is the the first block;
uint32_t *get_prev_block(uint32_t *ptr);

//functions need to be modified

//initialize a free block,ptr: the first word of the free block,size:the number of words in the block
void write_free_block(uint32_t *ptr, uint32_t word_size);
//ptr: the first word of the allocated block,size:the number of words in the block
void write_allocated_block(uint32_t *ptr, uint32_t word_size);
//get next free block in the segregate list,ptr is the header of the block,return null if it is the last block
uint32_t *get_next_free_block(uint32_t *ptr);
//get the previous block in the segregate list,ptr is the header of the block,return null if it is the first block
uint32_t *get_prev_free_block(uint32_t *ptr);
//set next free block in the segregate list,ptr is the header of the block,return null if it is the last block
void set_next_free_block(uint32_t *ptr, uint32_t *address);
//set the previous block in the segregate list,ptr is the header of the block,return null if it is the first block
void set_prev_free_block(uint32_t *ptr, uint32_t *address);
//expand the heap when the allocated memory has run out and return the header of the free block
uint32_t *expand_heap(uint32_t need_bytes);
//get needed size(this function is used as a auxillary function for malloc to determine the needed size)
uint32_t get_needed_word_number(uint32_t byteNumber);
//write and split the block when allocation happens
uint32_t *write_split(uint32_t *ptr, int word_size);
//find the free block in the segregate list,return null if not find
uint32_t *find_free_block(uint32_t word_size);
//return the index in the segregate list according to the word size,returns -1 if the size is too big
int get_index(uint32_t word_size);
//insert a block into the segregate free list
void insert_into_free_list(uint32_t *ptr, int index);
//remove a block from the segregate free list
void remove_from_free_list(uint32_t *ptr);
int check_tailer_size(const char *str, int index);

uint32_t my_pow(uint32_t x, uint32_t y);

void printinfo(uint32_t *ptr, const char *str)
{
    puts(str);
    printf("ptr:%p\n", ptr);
    printf("size:%d\n", get_block_size(ptr));
    printf("is allocated %d\n", is_block_allocated(ptr));
}

int check_tailer_size(const char *str, int index)
{
    if (get_block_size(tailer) != 0)
    {
        printf("tailer's size is %d\n", get_block_size(tailer));
        printf("The %s index is %d\n", str, index);
        return 0;
    }
    printf("called!\n");
    return 1;
}

int iterate(int size_print,const char* str)
{
    uint32_t *ptr = header;
    
    while (ptr != tailer)
    {
        ptr = get_next_block(ptr);
        if (ptr > tailer || ptr < header||((get_block_size(ptr)<4)&&(ptr!=header)&&(ptr!=tailer)))
        {
            puts(str);
            printf("error at %d size\n", size_print);
            printf("size:%d\n",get_block_size(ptr));
            printf("header:%p\n",header);
            printf("tailer:%p\n",tailer);
            printinfo(ptr,"detailed information for the pointer\n");

            return 0;
        }
    }
    return 1;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    header = (uint32_t *)mem_sbrk(4 * WORDSIZE);
    if ((int)header == -1)
        return -1;

    //mark the begining of the list
    header += 1;
    set_begin_block(header);
    tailer = header + 2;

    set_end_block(tailer);

    for (int i = 0; i < N; i++)
        segregated_list[i] = NULL;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    
    uint32_t word_number = get_needed_word_number(size);

    uint32_t *ptr = find_free_block(word_number);

    if (!ptr)
    {
        ptr = expand_heap(word_number * WORDSIZE);
    }

    ptr = write_split(ptr, word_number);
    

    ptr += 3;
    return (void *)ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */

void mm_free(void *ptr)
{

    uint32_t *this_header = ((uint32_t *)ptr) - 3;

    uint32_t *prev_header = get_prev_block(this_header);

    uint32_t *next_header = get_next_block(this_header);
    

    uint32_t this_size = get_block_size(this_header);
    uint32_t prev_size = get_block_size(prev_header);
    uint32_t next_size = get_block_size(next_header);
    int is_prevblock_allocated = 1, is_nextblock_allocated = 1;

    is_prevblock_allocated = is_block_allocated(prev_header);
    is_nextblock_allocated = is_block_allocated(next_header);

    if (!is_prevblock_allocated && !is_nextblock_allocated)
    {
        uint32_t total_size = next_size + prev_size + this_size;
        remove_from_free_list(prev_header);

        remove_from_free_list(next_header);

        write_free_block(prev_header, total_size);

        insert_into_free_list(prev_header, get_index(total_size));

    }
    else if (is_prevblock_allocated && !is_nextblock_allocated)
    {
        uint32_t total_size = next_size + this_size;
        remove_from_free_list(next_header);
        write_free_block(this_header, total_size);
        insert_into_free_list(this_header, get_index(total_size));

    }
    else if (!is_prevblock_allocated && is_nextblock_allocated)
    {

        uint32_t total_size = prev_size + this_size;
        remove_from_free_list(prev_header);
        write_free_block(prev_header, total_size);

        insert_into_free_list(prev_header, get_index(total_size));

    }
    else if (is_prevblock_allocated && is_nextblock_allocated)
    {

        uint32_t total_size = this_size;

        write_free_block(this_header, total_size);

        insert_into_free_list(this_header, get_index(total_size));

    }


    
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    void *oldptr = ptr;
    void *newptr;
    uint32_t this_block_size = get_block_size((uint32_t *)ptr - 3);
    uint32_t need_block_size=get_needed_word_number(size);
    if(this_block_size>=need_block_size)
        return oldptr;    

    //optimization(very important,otherwise the speed will be very low)
    newptr = mm_malloc(size*2);
    if (newptr == NULL)
        return NULL;

    memcpy(newptr, oldptr, size);
    mm_free(oldptr);
    return newptr;
}

int is_block_allocated(uint32_t *ptr)
{
    if (ptr)
        return (*ptr) & 0x1;
    else
    {
        return 0;
    }
}

void set_begin_block(uint32_t *ptr)
{
    //first write the first two word that mark the beginning of the list
    *ptr = 8;
    *ptr = (*ptr) | 0x1;
    ptr++;
    *ptr = 8;
    *ptr = (*ptr) | 0x1;
}

void set_end_block(uint32_t *ptr)
{
    *ptr = 0;
    *ptr = (*ptr) | 0x1;
}

void write_free_block(uint32_t *ptr, uint32_t word_size)
{
    *(ptr) = word_size * WORDSIZE;
    *(ptr + (word_size - 1)) = word_size * WORDSIZE;

    set_next_free_block(ptr, NULL);
    set_prev_free_block(ptr, NULL);
}

void write_allocated_block(uint32_t *ptr, uint32_t word_size)
{
    *ptr = (word_size * WORDSIZE) | (0x1);
    set_next_free_block(ptr, NULL);
    set_prev_free_block(ptr, NULL);
    *(ptr + (word_size - 1)) = (word_size * WORDSIZE) | (0x1);
}

uint32_t get_block_size(uint32_t *ptr)
{
    if (ptr)
    {
        return ((*ptr) & (~0x7)) / WORDSIZE;
    }
    else
    {
        return 0;
    }
}

uint32_t *get_tailer_from_header(uint32_t *ptr)
{

    uint32_t word_size = get_block_size(ptr);

    return ptr + (word_size - 1);
}

uint32_t *get_header_from_tailer(uint32_t *ptr)
{

    uint32_t word_size = get_block_size(ptr);

    return ptr - ((word_size - 1));
}

uint32_t *get_next_block(uint32_t *ptr)
{

    if (ptr == tailer)
        return NULL;
    uint32_t word_size = get_block_size(ptr);

    return ptr + word_size;
}

uint32_t *get_prev_block(uint32_t *ptr)
{

    if (ptr == header)
        return NULL;

    return get_header_from_tailer(ptr - 1);
}

uint32_t my_pow(uint32_t x, uint32_t y)
{
    uint32_t t = 1;
    for (uint32_t i = 0; i < y; i++)
        t = t * x;
    return t;
}

int get_index(uint32_t word_size)
{

    uint32_t need_bytes = word_size * WORDSIZE;
    for (int i = 0; i < N; i++)
        if (my_pow(2, i + 1) >= need_bytes)
            return i;
    return 0;
}

uint32_t *get_next_free_block(uint32_t *ptr)
{
    if (ptr)
    {
        return (uint32_t *)*(ptr + 1);
    }
    return NULL;
}

uint32_t *get_prev_free_block(uint32_t *ptr)
{
    if (ptr)
    {
        return (uint32_t *)*(ptr + 2);
    }
    return NULL;
}

void set_next_free_block(uint32_t *ptr, uint32_t *address)
{

    if (ptr)
    {

        *(uint32_t **)(ptr + 1) = address;
    }
}

void set_prev_free_block(uint32_t *ptr, uint32_t *address)
{

    if (ptr)
    {
        *(uint32_t **)(ptr + 2) = address;
    }
}

void insert_into_free_list(uint32_t *ptr, int index)
{
    if (index == 0)
    {
        fprintf(STDERR_FILENO, "fail to insert the block because of the size problem");
        exit(0);
    }
    if (get_block_size(ptr) == 0)
        return;
    uint32_t *this_header = segregated_list[index];
    if (this_header == NULL)
    {
        segregated_list[index] = ptr;
        set_next_free_block(ptr, NULL);
        set_prev_free_block(ptr, NULL);
        return;
    }
    segregated_list[index] = ptr;
    set_prev_free_block(ptr, NULL);
    set_next_free_block(ptr, this_header);
    set_prev_free_block(this_header, ptr);

    return;
}

void remove_from_free_list(uint32_t *ptr)
{
    uint32_t *prev_block = get_prev_free_block(ptr);
    uint32_t *next_block = get_next_free_block(ptr);

    int index = get_index(get_block_size(ptr));

    if (ptr == segregated_list[index])
    {
        if (prev_block == NULL && next_block == NULL)
        {

            int index = get_index(get_block_size(ptr));
            segregated_list[index] = NULL;
            return;
        }

        segregated_list[index] = next_block;
        set_prev_free_block(next_block, NULL);
        set_next_free_block(ptr, NULL);
        set_prev_free_block(ptr, NULL);
        return;
    }
    set_next_free_block(prev_block, next_block);
    set_prev_free_block(next_block, prev_block);
    set_next_free_block(ptr, NULL);
    set_prev_free_block(ptr, NULL);
}

uint32_t *find_free_block(uint32_t word_size)
{
    int begin_index = get_index(word_size);

    for (int i = begin_index; i < N; i++)
    {
        uint32_t *this_header = segregated_list[i];
        while (this_header != NULL)
        {
            uint32_t this_block_size = get_block_size(this_header);

            if (this_block_size >= word_size)
                return this_header;
            this_header = get_next_free_block(this_header);
        }
    }
    return NULL;
}

uint32_t *expand_heap(uint32_t need_bytes)
{

    uint32_t word_size = need_bytes / WORDSIZE;
    void *ptr = mem_sbrk(word_size * WORDSIZE);
    uint32_t *tmp_tailer = tailer;
    tailer += word_size;

    write_free_block(tmp_tailer, word_size);

    uint32_t *prev_header = get_header_from_tailer(tmp_tailer - 1);

    if (!is_block_allocated(prev_header))
    {

        //remove the previous header from the free list
        remove_from_free_list(prev_header);

        //initialize the free block
        int prev_word_size = get_block_size(prev_header);
        write_free_block(prev_header, prev_word_size + word_size);

        //add the new previous block to the free list
        int index = get_index(prev_word_size + word_size);

        insert_into_free_list(prev_header, index);

        //reset the tailer
        set_end_block(tailer);

        return prev_header;
    }

    //add the new allocated block into the segregate list
    int index = get_index(word_size);

    insert_into_free_list(tmp_tailer, index);

    //reset the tailer
    set_end_block(tailer);

    return ((uint32_t *)ptr) - 1;
}

uint32_t get_needed_word_number(uint32_t byteNumber)
{
    uint32_t word_number = ((byteNumber + 3) & (~0x3)) / WORDSIZE;
    if ((word_number) % 2 == 0)
        return word_number + 4;
    else
    {
        return word_number + 5;
    }
}

uint32_t *write_split(uint32_t *ptr, int word_size)
{

    remove_from_free_list(ptr);

    uint32_t block_word_num = get_block_size(ptr);
    if (block_word_num - word_size >= 4)
    {

        uint32_t remain_word_num = block_word_num - word_size;
        write_allocated_block(ptr, word_size);
        write_free_block(ptr + word_size, remain_word_num);
        int index = get_index(remain_word_num);
        insert_into_free_list(ptr + word_size, index);

        return ptr;
    }
    else
    {
        write_allocated_block(ptr, block_word_num);
        return ptr;
    }
    return NULL;
}

