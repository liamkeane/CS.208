/*
 * CS 208 Lab 4: Malloc Lab
 *
 * <Liam Keane (keanel)>
 * <Marcos Acero (acerom)>
 *
 * Simple allocator based on implicit free lists, first fit search,
 * and boundary tag coalescing.
 *
 * Each block has header and footer of the form:
 *
 *      63                  4  3  2  1  0
 *      ------------------------------------
 *     | s  s  s  s  ... s  s  0  0  0  a/f |
 *      ------------------------------------
 *
 * where s are the meaningful size bits and a/f is 1
 * if and only if the block is allocated. The list has the following form:
 *
 * begin                                                             end
 * heap                                                             heap
 *  -----------------------------------------------------------------
 * |  pad   | hdr(16:a) | ftr(16:a) | zero or more usr blks | hdr(0:a) |
 *  -----------------------------------------------------------------
 *          |       prologue        |                       | epilogue |
 *          |         block         |                       | block    |
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

/* Basic constants and macros */
#define WSIZE       8       /* word size (bytes) */
#define DSIZE       16      /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    16      /* overhead of header and footer (bytes) */
#define BLKMIN      32      /* minimum block size of our allocator (hdr + ftr + payload = ?) */

/* NOTE: feel free to replace these macros with helper functions and/or
 * add new ones that will be useful for you. Just make sure you think
 * carefully about why these work the way they do
 */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))

/* Perform unscaled pointer arithmetic (the char* makes it unscaled) */
#define PADD(p, val) ((char *)(p) + (val))
#define PSUB(p, val) ((char *)(p) - (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0xf)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
/* L: PADD  is adding to the bp the size given by the header minus the hdr and ftr size */
#define HDRP(bp)       (PSUB(bp, WSIZE))
#define FTRP(bp)       (PADD(bp, GET_SIZE(HDRP(bp)) - DSIZE))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  (PADD(bp, GET_SIZE(HDRP(bp))))
#define PREV_BLKP(bp)  (PSUB(bp, GET_SIZE((PSUB(bp, DSIZE)))))

/* L: Given block ptr bp, return allocation status of previous block and next block */
/* L: Should be useful when implementing coalesce */
#define GET_PALLOC(bp) (GET_ALLOC(HDRP(PREV_BLKP(bp))))
#define GET_NALLOC(bp) (GET_ALLOC(HDRP(NEXT_BLKP(bp))))

/* Rewrites the head pointer to bp */
#define HEADPTR(bp) (PUT(head, (size_t)(bp)))

/* Gets the next and previous pointers in the explicit free list */
#define NEXT_FREE(bp) ((void *)(bp))
#define PREV_FREE(bp) ((void *)(PADD(bp, WSIZE)))

/* Dereferences the next and previous pointers in the explicit free list */
#define GOTO_NEXT_FREE(bp) ((void *)(GET(bp)))
#define GOTO_PREV_FREE(bp) ((void *)(GET(PADD(bp, WSIZE))))

/* Global variables */

// Pointer to first block
static void *heap_start = NULL;

// Pointer to list head node of explicit free list
static void *head = NULL;

/* Function prototypes for internal helper routines */

static bool check_heap(int lineno);
static void print_heap();
static void print_block(void *bp);
static bool check_block(int lineno, void *bp);
static void *extend_heap(size_t size);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static size_t max(size_t x, size_t y);

/*
 * Set the pointer to the next free block.
 */
void setNextFree(void *bp, void *new_ptr) {
    PUT(bp, (size_t)new_ptr);
}

/*
 * Set the pointer to the previous free block.
 */
void setPrevFree(void *bp, void *new_ptr) {
    PUT(PADD(bp, WSIZE), (size_t)new_ptr);
}

/*
 * Insert free block into explicit free list
 * Cases: adding when there is or isn't anything in the list.
 */
void insertBLK(void *bp) {
    setNextFree(bp, head);
    if (head != NULL) {
        setPrevFree(head, bp);
    } 
    setPrevFree(bp, NULL);
    head = bp;
}

/*
 * Remove free block from explicit free list
 * Cases: removing from the end, beginning, and the middle.
 */
void removeBLK(void *bp){
    if (GOTO_PREV_FREE(bp)) {   /* Does the current block have a previous? (is prev null?) */
        setNextFree(GOTO_PREV_FREE(bp), GOTO_NEXT_FREE(bp));
    } else {    /* In the case w/o a previous, the head needs to be set */
        head = GOTO_NEXT_FREE(bp);
    }
    if (GOTO_NEXT_FREE(bp)) {   /* Does the current block have a next? (is next null?) */
        setPrevFree(GOTO_NEXT_FREE(bp), GOTO_PREV_FREE(bp));
    }
}

/*
 * mm_init -- Initalizes an empty heap and sets the header of the free list to null.
 */
int mm_init(void) {
    head = NULL;

    /* create the initial empty heap */
    if ((heap_start = mem_sbrk(4 * WSIZE)) == NULL)
        return -1;
    
    PUT(heap_start, 0);                                 /* alignment padding */
    PUT(PADD(heap_start, WSIZE), PACK(OVERHEAD, 1));    /* prologue header */
    PUT(PADD(heap_start, DSIZE), PACK(OVERHEAD, 1));    /* prologue footer */
    PUT(PADD(heap_start, WSIZE + DSIZE), PACK(0, 1));   /* epilogue header */

    heap_start = PADD(heap_start, DSIZE); /* start the heap at the (size 0) payload of the prologue block */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL){
        return -1;
    }

    return 0;
}

/*
 * mm_malloc -- Locates a block of sufficent size using first fit search, allocates block with given size. 
 *              If not enough space avaliable, extend the heap, then allocate.
 */
void *mm_malloc(size_t size) {
    size_t asize;      /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size <= 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. (If size is less than min, round up) */
    if (size <= DSIZE) {
        asize = DSIZE + OVERHEAD;
    } else {
        /* Add overhead and then round up to nearest multiple of double-word alignment */
        asize = DSIZE * ((size + (OVERHEAD) + (DSIZE - 1)) / DSIZE);
    }

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = max(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;

    place(bp, asize);
    return bp;
}

/*
 * mm_free -- Deallocates the block at the given pointer, coalesces surrounding blocks if necessary,
 *            and initalize the freed block into the free list.
 */
void mm_free(void *bp) {
    PUT(HDRP(bp), GET_SIZE(HDRP(bp)));
    PUT(FTRP(bp), GET_SIZE(FTRP(bp)));
    coalesce(bp);
}

/* The remaining routines are internal helper routines */

/*
 * place -- Allocates a block of asize bytes at start of free block bp
 *          and check to see if splitting is required if remaining free block
 *          after allocation is at least 32 bytes.
 */
static void place(void *bp, size_t asize) {

    if ((GET_SIZE(HDRP(bp)) - asize) >= BLKMIN) {   /* Will the remaining split free block be at least BLKMIN? */
        // places the new header for the remaining free block after what will be the footer of newly allocated block
        PUT(bp + (asize - 8), PACK((GET_SIZE(HDRP(bp)) - asize), 0));
        // replaces the footer for the remaining free block with the correct size
        PUT((FTRP(bp)), PACK((GET_SIZE(HDRP(bp)) - asize), 0));

        removeBLK(bp);
        insertBLK(bp + asize);

        // replaces the header of the free block with the new (allocated) header 
        PUT(HDRP(bp), PACK(asize, 1));
        // places the footer of the newly allocated block
        PUT((bp + (asize - 16)), PACK(asize, 1));
        return;
    }
    // no splitting necessary, allocate the rest of the block
    removeBLK(bp);  /* resets pointers of next, prev, and head if necessary */
    PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
}

/*
 * coalesce -- Takes a pointer to free block and checks if the surrounding blocks in memory are free in order to coalesce.
 *             Adjusts the pointers and the value of head as necessary, and place the block at the beginning of the free list.
 */
static void *coalesce(void *bp) {
    size_t total = 0;
    if (!(GET_PALLOC(bp))) {
        if (!(GET_NALLOC(bp))) {    /* Handles the case where both the previous and next block are free */

            /* Remove the previous and next blocks (in memory) from the free list */
            removeBLK(PREV_BLKP(bp));
            removeBLK(NEXT_BLKP(bp));
            
            /* Changes boundary tags */
            total = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(total, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(total, 0));

        } else {    /* Handles the case when just the previous block is free */

            /* Remove previous block from the free list */
            removeBLK(PREV_BLKP(bp));

            /* Changes boundary tags */
            total = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(PREV_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(total, 0));
            PUT(FTRP(bp), PACK(total, 0));

        }
        
        /* Insert coalesced block into the free list */
        insertBLK(PREV_BLKP(bp));
        return PREV_BLKP(bp);
        
    }
    if (!(GET_NALLOC(bp))) {    /* Handles the case when just the next block is free */

        /* Remove the next block from the free list */
        removeBLK(NEXT_BLKP(bp));

        /* Changes boundary tags */
        total = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(total, 0));
        PUT(FTRP(bp), PACK(total, 0));
        
    }
    
    /* Insert coalesced block (or the current block) into the free list */
    insertBLK(bp);
    return bp;
}

/*
 * find_fit - Find a fit for a block with asize bytes.
 */
static void *find_fit(size_t asize) {
    /* search from the start of the free list to the end */
    for (char *cur_block = head; cur_block != NULL; cur_block = GOTO_NEXT_FREE(cur_block)) {
        if (asize <= GET_SIZE(HDRP(cur_block)))
            return cur_block;
    }
    return NULL;
}

/*
 * extend_heap - Extend heap with free block and return its block pointer.
 */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = words * WSIZE;
    if (words % 2 == 1)
        size += WSIZE;
    if ((long)(bp = mem_sbrk(size)) < 0)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */
   
    return coalesce(bp); 
}

/*
 * check_heap -- Performs basic heap consistency checks for an implicit free list allocator
 * and prints out all blocks in the heap in memory order.
 * Checks include proper prologue and epilogue, alignment, and matching header and footer
 * Takes a line number (to give the output an identifying tag).
 */
static bool check_heap(int line) {
    char *bp;

    if ((GET_SIZE(HDRP(heap_start)) != DSIZE) || !GET_ALLOC(HDRP(heap_start))) {
        printf("(check_heap at line %d) Error: bad prologue header\n", line);
        return false;
    }

    for (bp = heap_start; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!check_block(line, bp)) {
            return false;
        }
    }

    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp)))) {
        printf("(check_heap at line %d) Error: bad epilogue header\n", line);
        return false;
    }

    return true;
}

/*
 * check_block -- Checks a block for alignment and matching header and footer.
 */
static bool check_block(int line, void *bp) {
    if ((size_t)bp % DSIZE) {
        printf("(check_heap at line %d) Error: %p is not double-word aligned\n", line, bp);
        return false;
    }
    if (GET(HDRP(bp)) != GET(FTRP(bp))) {
        printf("(check_heap at line %d) Error: header does not match footer\n", line);
        return false;
    }
    return true;
}

/*
 * print_heap -- Prints out the current state of the implicit free list.
 */
static void print_heap() {
    char *bp;

    printf("Heap (%p):\n", heap_start);

    for (bp = heap_start; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        print_block(bp);
    }

    print_block(bp);
}

/*
 * print_block -- Prints out the current state of a block.
 */
static void print_block(void *bp) {
    size_t hsize, halloc, fsize, falloc, next_ptr, prev_ptr;

    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));
    next_ptr = (size_t)GET(bp);
    prev_ptr = (size_t)GET(PADD(bp, WSIZE));

    if (hsize == 0) {
        printf("%p: End of free list\n", bp);
        return;
    }
    if (!falloc) {  // simple condition checks whether or not block is free in order to display next and prev pointers
        printf("%p: header: [%ld:%c] footer: [%ld:%c] next_ptr: [0x%lx] prev_ptr: [0x%lx]\n", bp,
        hsize, (halloc ? 'a' : 'f'),
        fsize, (falloc ? 'a' : 'f'),
        next_ptr, prev_ptr);
    } else {
        printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp,
        hsize, (halloc ? 'a' : 'f'),
        fsize, (falloc ? 'a' : 'f'));
    }
    fflush(stdout);
}

/*
 * max -- Returns x if x > y, and y otherwise.
 */
static size_t max(size_t x, size_t y) {
    return (x > y) ? x : y;
}