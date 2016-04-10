/*
* mm.c
*
*
* Name: Ningna Wang
* AndrewID: ningnaw
*
* For implicit free list, please checkout mm-textbook.c
*
*
* This solution is implemented by Segregated Free List (Seglist Allocator).
* Each size class of free blocks has its own free list
*
*
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
* in, remove the #define DEBUG line. */
// #define DEBUG

#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
# define CHECKHEAP(lineno) mm_checkheap(lineno)
# define CHECKBLOCK(bp) check_block(bp)
# define PRINT_seglists() print_seglists()
# define PRINT_seglist_by_class(bClass) print_seglist_by_class(bClass)
# define PRINT_prologue_block() print_prologue_block()
# define PRINT_epilogue_block() print_epilogue_block()
# define PRINT_block(bp) print_block(bp)
# define PRINT_free_block(bp) print_free_block(bp)

#else
# define dbg_printf(...)
# define CHECKHEAP(lineno)
# define CHECKBLOCK(bp)
# define PRINT_seglists()
# define PRINT_seglist_by_class(bClass)
# define PRINT_prologue_block()
# define PRINT_epilogue_block()
# define PRINT_block(bp)
# define PRINT_free_block(bp)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */



/*
 * For Basic Defines
 */



/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */

/* size of seglist (number of roots in seglist) */
#define SIZE_SEGLIST 20

/* mininum block size */
#define MIN_BLK_SIZE 16  /* since the maximun heap size is 2^32 bytes */

#define MAX(x, y) ((x) > (y)? (x) : (y))


/*
 * For Both Allocated Blocks and Free Blocks
 */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* write header without modify prev alloc flag */
#define PUT_HEAD(p, val) (GET(p) = (GET(p) & 0x2) | (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, read the size and allocated fields */
#define GET_BLK_SIZE(bp)  (GET(HDRP(bp)) & ~0x7)
#define GET_BLK_ALLOC(bp) (GET(HDRP(bp)) & 0x1)

/* Read the prev block allocated fields from address p */
#define GET_PREVBLK_ALLOC(p) ((GET(p) & 0x2) >> 1)
#define SET_PREVBLK_ALLOC(p) (GET(p) |= 0x2)
#define UNSET_PREVBLK_ALLOC(p) (GET(p) &= ~0x2)


/* Given block ptr bp, return the value of prev block's allocation flag */
/* flag: 1 allocated, 0 free */
#define GET_BLK_PREVBLK_ALLOC(bp) ((GET(HDRP(bp)) & 0x2) >> 1)
#define SET_BLK_PREVBLK_ALLOC(bp) (GET(HDRP(bp)) |= 0x2)


/* Given block ptr bp, set/unset its next block's prev allocation flag */
#define SET_NEXTBLK_ALLOC_HD(bp) (GET(HDRP(NEXT_BLKP(bp))) |= 0x2)
#define SET_NEXTBLK_ALLOC_FT(bp) (GET(FTRP(NEXT_BLKP(bp))) |= 0x2)
#define UNSET_NEXTBLK_ALLOC_HD(bp) (GET(HDRP(NEXT_BLKP(bp))) &= ~0x2)
#define UNSET_NEXTBLK_ALLOC_FT(bp) (GET(FTRP(NEXT_BLKP(bp))) &= ~0x2)


/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


/* Given free block ptr bp, return offset pointer represents next or prev block */
#define NEXT_FREE_BLK_OFFSET_P(bp) ((unsigned int *)(bp))
#define PREV_FREE_BLK_OFFSET_P(bp) ((unsigned int *)((char *)(bp) + WSIZE))

/* 4-byte unsigned int: using for init next/prev pointer of free blocks */
#define INT_INIT 0U

/*
 * Global variables
 */

/* Pointer to first block */
static char *heap_listp = 0;

/* block pointer type: (void *) */
typedef void *Bptr;

/* 4-byte unsigned int: using for init next/prev pointer of free blocks */
// static unsigned int INT_INIT = 0U;

/* put seglist at the beginning of the heap */
/* segregated free list */
static unsigned int *seg_listp = 0; // point to first block offset of Seglists on heap




/*
 * Function prototypes for internal helper routines
 */
static Bptr extend_heap(size_t words);
static void place(Bptr bp, size_t asize);
static Bptr find_fit(size_t asize);
static Bptr coalesce(Bptr bp);
static inline void seglists_init(void);
// static inline char *get_seglist_by_bClass(int bSize);
static inline unsigned int *get_seglist_by_bClass(int bSize);
static inline unsigned int get_bClass_by_bSize(int bSize);
static inline void remove_fblock_from_seglist(Bptr bp);
static inline void add_fblock_to_seglist(Bptr bp, unsigned int bSize);

/*
 * Helper functions
 */
static inline Bptr int2pointer(unsigned int offset);
static inline unsigned int pointer2int(Bptr p);
static inline void print_seglists(void);
static inline void print_seglist_by_class(int bClass);
static inline void print_prologue_block(void);
static inline void print_epilogue_block(void);
static inline void print_block(Bptr bp);
static inline void print_free_block(Bptr bp);
static inline void check_block(Bptr bp);
static int in_heap(const Bptr p);
static int aligned(const Bptr p);



/*
 * Initialize the memory manager
 *
 *
 * return:  -1 on error, 0 on success.
 */
int mm_init(void) {

    dbg_printf("------- In mm_init function -------\n");

    /* Create the initial empty heap */

    /* create the initial empty Seglists */
    // put root Bptr of each Seglist at the beginning of the heap
    // create a array A of Bptr, each element in this array is a Bptr
    // that point to the root block of Seglist
    // if ((seg_listp = mem_sbrk(ALIGN(sizeof(Bptr) * SIZE_SEGLIST)
    //                             + MIN_BLK_SIZE)) == (void *)-1)
    if ((seg_listp = mem_sbrk(ALIGN(sizeof(int) * SIZE_SEGLIST)
                                + MIN_BLK_SIZE)) == (void *)-1)
        return -1;
    // Init Seglists in heap
    seglists_init();

    dbg_printf("The start address of heap is %p \n", seg_listp);

    // heap_listp = (char *)ALIGN(seg_listp + sizeof(Bptr) * SIZE_SEGLIST);
    heap_listp = (char *)ALIGN(seg_listp + SIZE_SEGLIST);

    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2 * WSIZE);


    // set epilogue block's prev alloc flag
    SET_NEXTBLK_ALLOC_HD(heap_listp);

    CHECKHEAP(1);
    // CHECKBLOCK(heap_listp);



    //  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    // if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    //     return -1;


    return 0;
}

/*
 * malloc
 */
Bptr malloc(size_t size) {
    // return NULL;

    dbg_printf("----------------- In malloc function -----------------\n");
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    if (heap_listp == 0){
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);

        dbg_printf("-------------- In malloc function END --------------\n");

        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    dbg_printf("----------------- In malloc function END -----------------\n");

    return bp;
}

/*
 * free:    free an allocated block, given block pointer bp
 *          This routine is only guaranteed to work when the passed
 *          pointer (ptr) was returned by an earlier call to malloc,
 *          calloc, or realloc and has not yet been freed.
 *          free(NULL) has no effect.
 */
void mm_free(Bptr bp) {
    dbg_printf("----------------- In mm_free function ---------------\n");

    /* handle free(NULL) */
    if(!bp) return;

    /* handle bp not in heap */
    if (!in_heap(bp) || !aligned(bp)) return;

    dbg_printf("Let's print block %p before free \n", bp);
    PRINT_block(bp);
    CHECKBLOCK(bp);

    size_t size = GET_SIZE(HDRP(bp));

    if (size < MIN_BLK_SIZE) {
        dbg_printf("Error: block cannot be freed: size not right\n");
        exit(-1);
        return;
    }

    /* Reset header/footer */
    PUT_HEAD(HDRP(bp), PACK(size, 0));
    PUT_HEAD(FTRP(bp), PACK(size, 0));
    UNSET_NEXTBLK_ALLOC_HD(bp);
    UNSET_NEXTBLK_ALLOC_FT(bp);

    /* Initialize free block next/previous 4-byte pointer */
    PUT(NEXT_FREE_BLK_OFFSET_P(bp), INT_INIT);
    PUT(PREV_FREE_BLK_OFFSET_P(bp), INT_INIT);

    dbg_printf("----------------next block %p info --------\n", NEXT_BLKP(bp));
    PRINT_block(NEXT_BLKP(bp));
    CHECKBLOCK(NEXT_BLKP(bp));


    dbg_printf("Let's print block %p after free \n", bp);
    PRINT_free_block(bp);
    CHECKBLOCK(bp);

    Bptr new_bp = coalesce(bp);
    add_fblock_to_seglist(new_bp, GET_BLK_SIZE(new_bp));

    dbg_printf("Let's print block %p after coalesce \n", bp);
    PRINT_free_block(new_bp);
    CHECKBLOCK(new_bp);
}

/*
* realloc - you may want to look at mm-naive.c
*/
Bptr realloc(Bptr bp, size_t size) {
    dbg_printf("--------------------- In realloc function -------------------\n");

    size_t oldsize;
    size_t asize;
    Bptr new_bp;


    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        dbg_printf("size == 0 \n");
        mm_free(bp);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(bp == NULL) {
        dbg_printf("bp == NULL \n");
        return mm_malloc(size);
    }

    oldsize = GET_SIZE(HDRP(bp));
    asize = MAX(MIN_BLK_SIZE, ALIGN(size));

    /* Case 1: the return address of the new block is the same as the 
               old block */
    
    /* Case 1.1: If the new size was smaller than the old size */
    if (asize <= oldsize) {
        dbg_printf("Case 1.1: asize %zu <= oldsize %zu \n", asize, oldsize);

        PRINT_block(bp);
        CHECKBLOCK(bp);

        place(bp, asize);
        return bp;
    }

    /* Case 1.2: there was free space after the old block  */
    else if (!GET_BLK_ALLOC(NEXT_BLKP(bp))) {
        size_t bSize = GET_BLK_SIZE(NEXT_BLKP(bp)) + oldsize;
        dbg_printf("Case 1.2: there was free space after the old block\n");

        if (asize <= bSize) {
            dbg_printf("asize %zu <= extended block size %zu \n", asize, bSize);
            remove_fblock_from_seglist(NEXT_BLKP(bp));
            PUT_HEAD(HDRP(bp), PACK(bSize, 1));
            int alloc = GET_BLK_PREVBLK_ALLOC(bp);
            if (alloc) {
               SET_PREVBLK_ALLOC(FTRP(bp));
            }
            else {
                UNSET_PREVBLK_ALLOC(FTRP(bp));
            }
            PUT_HEAD(FTRP(bp), PACK(bSize, 1));

            PRINT_block(bp);
            CHECKBLOCK(bp);

            place(bp, asize);
            return bp;
        }
    }

    /* Case 2: the returned address is different than the address passed 
                in, the old block has been freed and should not be used, 
                freed, or passed to realloc again*/

    dbg_printf("Case 2: the returned address is different than old block\n")

    new_bp = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!new_bp) {
        return 0;
    }

    // /* Copy the old data. */
    // oldsize = GET_SIZE(HDRP(ptr));

    dbg_printf("Copy the old data:\n");
    dbg_printf("old size %zu \n", oldsize);
    dbg_printf("current size %zu \n", size);

    // if(size < oldsize) oldsize = size;
    memcpy(new_bp, bp, oldsize);

    /* Free the old block. */
    mm_free(bp);


    dbg_printf("--------------------- In realloc function END-------------------\n");
    return new_bp;
}

/*
* calloc - you may want to look at mm-naive.c
* This function is not tested by mdriver, but it is
* needed to run the traces.
*/
Bptr calloc(size_t nmemb, size_t size) {

    size_t bytes = nmemb * size;  
    Bptr newptr;  
  
    newptr = mm_malloc(bytes);  
    memset(newptr, 0, bytes);  
  
    return newptr;
}



/******************************************************
 ** Function prototypes for internal helper routines **
 ******************************************************/


/*
* coalesce - Boundary tag coalescing. Return ptr to coalesced block
*/
static Bptr coalesce(Bptr bp) {
    dbg_printf("--------------- In coalesce function ---------------\n");

    /* get prev and next block pointer */
    Bptr prevB = PREV_BLKP(bp);
    Bptr nextB = NEXT_BLKP(bp);

    // size_t prev_alloc = GET_ALLOC(FTRP(prevB));
    size_t prev_alloc = GET_BLK_PREVBLK_ALLOC(bp);
    size_t next_alloc = GET_ALLOC(HDRP(nextB));
    size_t size = GET_SIZE(HDRP(bp));

    dbg_printf("Let's check block %p before coalesce: \n", bp);
    PRINT_free_block(bp);
    CHECKBLOCK(bp);

    if (prev_alloc && next_alloc) {            /* Case 1 */
        dbg_printf("Case1: no coalesce \n");
        dbg_printf("--------------- In coalesce function END---------------\n");
        return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        dbg_printf("Case2: coalesce next block \n");
        remove_fblock_from_seglist(nextB);
        size += GET_SIZE(HDRP(nextB));
        PUT_HEAD(HDRP(bp), PACK(size, 0));
        int alloc = GET_BLK_PREVBLK_ALLOC(bp);
        if (alloc) {
           SET_PREVBLK_ALLOC(FTRP(bp));
        }
        else {
            UNSET_PREVBLK_ALLOC(FTRP(bp));
        }
        PUT_HEAD(FTRP(bp), PACK(size, 0));

    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        dbg_printf("Case3: coalesce prev block %p \n", prevB);
        PRINT_block(prevB);
        remove_fblock_from_seglist(prevB);

        /* setup new free block */
        size += GET_SIZE(HDRP(prevB));
        // PUT_HEAD(FTRP(bp), PACK(size, 0));
        // PUT_HEAD(HDRP(prevB), PACK(size, 0));
        // bp = prevB;
        bp = prevB;
        PUT_HEAD(HDRP(bp), PACK(size, 0));
        int alloc = GET_BLK_PREVBLK_ALLOC(bp);
        if (alloc) {
            SET_PREVBLK_ALLOC(FTRP(bp));
        }
        else {
            UNSET_PREVBLK_ALLOC(FTRP(bp));
        }
        PUT_HEAD(FTRP(bp), PACK(size, 0));
    }

    else {                                     /* Case 4 */
        dbg_printf("Case4: coalesce prev + next block \n");
        remove_fblock_from_seglist(nextB);
        remove_fblock_from_seglist(prevB);

        /* setup new free block */
        size += GET_SIZE(HDRP(prevB)) + GET_SIZE(FTRP(nextB));

        // PUT_HEAD(HDRP(prevB), PACK(size, 0));
        // PUT_HEAD(FTRP(nextB), PACK(size, 0));
        // bp = prevB;
        bp = prevB;
        PUT_HEAD(HDRP(bp), PACK(size, 0));
        int alloc = GET_BLK_PREVBLK_ALLOC(bp);
        if (alloc) {
           SET_PREVBLK_ALLOC(FTRP(bp));
        }
        else {
            UNSET_PREVBLK_ALLOC(FTRP(bp));
        }
        PUT_HEAD(FTRP(bp), PACK(size, 0));
    }

    /* set free block's next/prev offset pointer */
    PUT(NEXT_FREE_BLK_OFFSET_P(bp), INT_INIT);
    PUT(PREV_FREE_BLK_OFFSET_P(bp), INT_INIT);

    // no need to add free block, only called after extend_heap(), free()
    // add_fblock_to_seglist(bp, GET_BLK_SIZE(bp));

    dbg_printf("Let's check block %p after coalesce: \n", bp);
    PRINT_free_block(bp);
    CHECKBLOCK(bp);

    dbg_printf("--------------- In coalesce function END ---------------\n");

    return bp;
}

/*
* place - Place block of asize bytes at start of free block bp
*         and split if remainder would be at least minimum block size
*
* usually call after find_fit() or extend_heap() functions
* and find_fit() will remove free block from seglist once find it
* so no need to remove free block in place() function anymore
*/
static void place(Bptr bp, size_t asize) {
    dbg_printf("----------------- In place function -----------------\n");

    size_t csize = GET_SIZE(HDRP(bp));
    size_t diff = csize - asize;

    dbg_printf("block size: %zu \n", csize);
    dbg_printf("content size: %zu \n", asize);
    dbg_printf("block pointer: %p \n", bp);

    // split when remainder is at least MIN_BLK_SIZE
    if (diff >= MIN_BLK_SIZE) {

        dbg_printf("remainder %zu >= MIN_BLK_SIZE, Split! \n", diff);
        PUT_HEAD(HDRP(bp), PACK(asize, 1));
        int alloc_1 = GET_BLK_PREVBLK_ALLOC(bp);
        if (alloc_1) {
            SET_PREVBLK_ALLOC(FTRP(bp));
        }
        else {
            UNSET_PREVBLK_ALLOC(FTRP(bp));
        }
        PUT_HEAD(FTRP(bp), PACK(asize, 1));
        SET_NEXTBLK_ALLOC_HD(bp);
        // SET_NEXTBLK_ALLOC_FT(bp); // not know the size of next block

        dbg_printf("========= print allocated block ========\n");
        PRINT_block(bp);
        CHECKBLOCK(bp);

        /* setup new free block */
        bp = NEXT_BLKP(bp);
        PUT_HEAD(HDRP(bp), PACK(diff, 0));
        int alloc_2 = GET_BLK_PREVBLK_ALLOC(bp);
        dbg_printf("alloc_2: %d \n", alloc_2);
        if (alloc_2) {
           SET_PREVBLK_ALLOC(FTRP(bp));
        }
        else {
            UNSET_PREVBLK_ALLOC(FTRP(bp));
        }
        PUT_HEAD(FTRP(bp), PACK(diff, 0));

        /* set free block's next/prev offset pointer */
        PUT(NEXT_FREE_BLK_OFFSET_P(bp), INT_INIT);
        PUT(PREV_FREE_BLK_OFFSET_P(bp), INT_INIT);


        // UNSET_NEXTBLK_ALLOC_HD(bp);
        // UNSET_NEXTBLK_ALLOC_FT(bp); // error if next block is epilogue 


        dbg_printf("========= print new free block ========\n");
        PRINT_free_block(bp);
        CHECKBLOCK(bp);

        add_fblock_to_seglist(bp, GET_BLK_SIZE(bp));
    }

    else {
        dbg_printf("remainder %zu < MIN_BLK_SIZE, Use all of it! \n", diff);

        PUT_HEAD(HDRP(bp), PACK(csize, 1));
        PUT_HEAD(FTRP(bp), PACK(csize, 1));
        SET_NEXTBLK_ALLOC_HD(bp);
        SET_NEXTBLK_ALLOC_FT(bp);

        dbg_printf("========= print free block after place ========\n");
        PRINT_block(bp);
        CHECKBLOCK(bp);
    }

    dbg_printf("----------------- In place function END-----------------\n");

}

/*
* find_fit - Find a fit for a block with asize bytes
*/
static Bptr find_fit(size_t asize) {

    dbg_printf("---------------- In find_fit function ------------- \n");
    unsigned int *seglist_class_offset;
    unsigned int *block_offset;


    int bClass = get_bClass_by_bSize(asize);

    // find the fit empty class
    for (int c = bClass; c < SIZE_SEGLIST; c++) {

        seglist_class_offset = get_seglist_by_bClass(c);
        /* get address of bptr by offset */
        Bptr bp = int2pointer(*seglist_class_offset);

        /* find the fit block */
        while (bp != NULL) {

            if (asize <= GET_BLK_SIZE(bp)) {
                dbg_printf("Find the fit block %p, size %d \n",
                            bp, GET_BLK_SIZE(bp));
                remove_fblock_from_seglist(bp);
                return bp;
            }

            /* check next block */
            block_offset = NEXT_FREE_BLK_OFFSET_P(bp);
            bp = int2pointer(*block_offset);
        }

        dbg_printf("cannot find fit block of class %d, go to next class \n", c);
        // get next class's first block offset
        // seglist_class_offset += 1;
        // bp = int2pointer(*seglist_class_offset);
    }


    /* not find the fit block in fit class */
    dbg_printf("cannot find the fit block \n");

    dbg_printf("---------------- In find_fit function END ------------- \n");

    return NULL;
}



/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static Bptr extend_heap(size_t words) {
    dbg_printf("---------------- In extend_heap function ------------- \n");

    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer */
    PUT_HEAD(HDRP(bp), PACK(size, 0));         /* Free block header */
    int alloc = GET_BLK_PREVBLK_ALLOC(bp);
    if (alloc) {
       SET_PREVBLK_ALLOC(FTRP(bp));
    }
    else {
        UNSET_PREVBLK_ALLOC(FTRP(bp));
    }
    PUT_HEAD(FTRP(bp), PACK(size, 0));         /* Free block footer */


    /* Initialize free block header/footer prev's block alloc flag */
    // SET_PREVBLK_ALLOC(HDRP(bp));
    // SET_PREVBLK_ALLOC(FTRP(bp));


    // /* Initialize free block next/previous 4-byte pointer */
    // PUT(NEXT_FREE_BLK_OFFSET_P(bp), INT_INIT);
    // PUT(PREV_FREE_BLK_OFFSET_P(bp), INT_INIT);


    /* Initialize the epilogue header and prev alloc flag */
    PUT_HEAD(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


static inline void add_fblock_to_seglist(Bptr bp, unsigned int bSize) {
    dbg_printf("------- In add_fblock_to_seglist function ---------- \n");

    int bClass = get_bClass_by_bSize(bSize);
    unsigned int *seglist = get_seglist_by_bClass(bClass);
    dbg_printf("Add new free block %p to seglist %p \n", bp, seglist);
    Bptr old_bp = int2pointer(*seglist);

    dbg_printf("Let's print the seglist %p before adding: %p \n", seglist, bp);
    PRINT_seglist_by_class(bClass);

    // this seglist has inside blocks
    if (old_bp) {
        dbg_printf("this seglist has inside blocks \n");
        PUT(NEXT_FREE_BLK_OFFSET_P(bp), *seglist);
        PUT(PREV_FREE_BLK_OFFSET_P(old_bp), pointer2int(bp));
    }

    dbg_printf("this seglist has no blocks \n");
    PUT(PREV_FREE_BLK_OFFSET_P(bp), INT_INIT);
    PUT(seglist, pointer2int(bp));

    dbg_printf("Let's print the seglist %p after adding: %p \n", seglist, bp);
    PRINT_seglist_by_class(bClass);
}


static inline void remove_fblock_from_seglist(Bptr bp) {
    dbg_printf("------- In remove_fblock_from_seglist function -------\n");

    unsigned int *prevB_offset_P = PREV_FREE_BLK_OFFSET_P(bp);
    unsigned int *nextB_offset_P = NEXT_FREE_BLK_OFFSET_P(bp);
    int bClass = get_bClass_by_bSize(GET_BLK_SIZE(bp));
    unsigned int *seglist = get_seglist_by_bClass(bClass);

    dbg_printf("Remove free block %p from seglist %p \n", bp, seglist);
    PRINT_free_block(bp);

    dbg_printf("Let's print the seglist %p before removing: %p \n", seglist, bp);
    PRINT_seglist_by_class(bClass);

    // if no prev block, update head of seglist
    if (*prevB_offset_P == INT_INIT) {

        dbg_printf("Update head block of seglist \n");

        PUT(seglist, *nextB_offset_P);
    }

    dbg_printf("Update inside block of seglist \n");

    // Bptr preb_bp = int2pointer(*prevB_offset_P);
    // dbg_printf("preb_bp: %p \n", preb_bp);
    // dbg_printf("*nextB_offset_P: %x \n", *nextB_offset_P);
    // PUT(NEXT_FREE_BLK_OFFSET_P(preb_bp), *nextB_offset_P);

    if (*prevB_offset_P) {
        dbg_printf("lalalla\n");
        Bptr prev_bp = int2pointer(*prevB_offset_P);
        dbg_printf("prev_bp: %p \n", prev_bp);
        dbg_printf("NEXT_FREE_BLK_OFFSET_P(prev_bp): %p \n", NEXT_FREE_BLK_OFFSET_P(prev_bp));
        dbg_printf("pointer2int(nextB_offset_P): %x \n", pointer2int(nextB_offset_P));

        PUT(NEXT_FREE_BLK_OFFSET_P(prev_bp), *nextB_offset_P);
    }

    if (*nextB_offset_P) {
        dbg_printf("bibibibi\n");
        Bptr next_bp = int2pointer(*nextB_offset_P);
        PUT(PREV_FREE_BLK_OFFSET_P(next_bp), *prevB_offset_P);
    }


    dbg_printf("Let's print the seglist %p after removing: %p \n", seglist, bp);
    PRINT_seglist_by_class(bClass);

}



static inline void seglists_init(void) {
    // dbg_printf("----- IN seglist_init -----\n");

    // memset(Seglist, 0, sizeof(Bptr) * SIZE_SEGLIST);

    for (int bClass = 0; bClass < SIZE_SEGLIST; bClass++) {

        // printf("bClass: %x \n", bClass);

        // char *seglist = get_seglist_by_bClass(bClass);
        unsigned int *seglist = get_seglist_by_bClass(bClass);

        // printf("seglist: %p \n", seglist);


        PUT(seglist, 0U);
    }

    // dbg_printf("----- IN seglist_init End -----\n");


}

static inline unsigned int *get_seglist_by_bClass(int bClass) {
    // dbg_printf("----- IN get_seglist_by_bClass -----\n");

    if (bClass != -1) {
        // return seg_listp + sizeof(Bptr) * bClass;
        return seg_listp + bClass;
    }
    else {

        #ifdef DEBUG
            printf("----- Error: bClass is < 0 -----\n");
        #endif
        return NULL;
    }

}


/*
 * Get block class by block size: using for index Seglists
 */
static inline unsigned int get_bClass_by_bSize(int bSize) {

    if (bSize < 0) {
        dbg_printf("Error: Your bSize %x is smaller than 0\n", bSize);
        exit(-1);
    }

    if (bSize < MIN_BLK_SIZE) {
        dbg_printf("Error: Your bSize %x is smaller than MIN_BLK_SIZE\n", bSize);
        exit(-1);
    }

    // int size = bSize / MIN_BLK_SIZE;
    int size = bSize / DSIZE;

    dbg_printf("block with bSize %d is size %d \n", bSize, size);

    /***** Seglist Instruction ****/
    // bSize:     size:       class:
    // 0-15       1=2^0         0
    // 16-23      2=2^1         1
    // 24-64      4=2^2         2
    // 65-128     8=2^3         3
    //  ...         ..          ..
    //            2^19         19


    int bClass = 0;

    // Select class(index) of Seglist
    while (bClass < SIZE_SEGLIST  && (size > 1)) {
        // dbg_printf("lallala \n");
        size >>= 1;
        bClass++;
    }

    dbg_printf("Block with bSize %d is class %d \n", bSize, bClass);
    return bClass;
}





/***************************
 * Check & Print functions *
 ***************************/

/*
 * mm_checkheap
 *
 * Check heap
 */
void mm_checkheap(int lineno) {

    if (!lineno) {
        printf("Not check the heap. \n");
        return;
    }

    printf("**************** Let's check the Heap **************** \n");


    // print seglists which are in the head of heap
    PRINT_seglists();

    char *tmp_ptr = heap_listp;

    // print prologue block
    PRINT_prologue_block();

    // check prologue block
    if (GET_BLK_SIZE(tmp_ptr) != DSIZE || !GET_BLK_ALLOC(tmp_ptr)) {
        printf("Error: the prologue block has some problems \n");
        exit(-1);
    }

    // print & check each block
    tmp_ptr = NEXT_BLKP(tmp_ptr);
    // print each block
    for (; GET_BLK_SIZE(tmp_ptr) > 0; tmp_ptr = NEXT_BLKP(tmp_ptr)) {
        // dbg_printf("tmp_ptr: %p \n", tmp_ptr);
        PRINT_block(tmp_ptr);
        CHECKBLOCK(tmp_ptr);
    }

    // print epilogue block
    PRINT_epilogue_block();

    // check epilogue block
    if (GET_BLK_SIZE(tmp_ptr) != 0 || !GET_BLK_ALLOC(tmp_ptr)) {
        printf("Error: the epilogue block has some problems \n");
        printf("GET_BLK_SIZE(tmp_ptr): %x, DSIZE: %x \n",
                GET_BLK_SIZE(tmp_ptr), DSIZE);
        printf("GET_BLK_ALLOC(tmp_ptr): %x \n", GET_BLK_ALLOC(tmp_ptr));

        exit(-1);
    }

    // check heap boundaries.


    printf("**************** Check the Heap End **************** \n");
}




/*
 * Check block
 */
static inline void check_block(Bptr bp) {

    printf("=== Let's check block %p ====  \n", bp);

    // Check each block’s address alignment.
    if (!aligned(bp)) {
        printf("The free block %p is not aligned\n", bp);
        exit(-1);
    }

    // Check heap boundaries
    // if (GET(NEXT_FREE_BLK_OFFSET_P(bp)) >

    // Check each block’s header and footer: size (minimum size, alignment),
    // previous/next allocate/free bit consistency, header and footer matching each other.
    if (GET(HDRP(bp)) != GET(FTRP(bp))) {
        printf("Block header %x does not match footer %x \n",
                GET(HDRP(bp)), GET(FTRP(bp)));
        exit(-1);
    }

    if (GET_BLK_SIZE(bp) < MIN_BLK_SIZE) {
        printf("Block size %x is smaller than minimum size %d \n",
                GET_BLK_SIZE(bp), MIN_BLK_SIZE);
        exit(-1);
    }

    if (GET_BLK_ALLOC(bp) != GET_BLK_PREVBLK_ALLOC(NEXT_BLKP(bp))) {
        printf("Current block flag %x does not match next block’s previous flag %x \n",
                GET_BLK_ALLOC(bp), GET_BLK_PREVBLK_ALLOC(NEXT_BLKP(bp)));
        exit(-1);
    }

    printf("====  Check free block %p End ==== \n", bp);

}


static inline void print_prologue_block(void) {
    printf("+++++++ Let's print Prologue Block +++++++ \n");

    Bptr bp = heap_listp;

    printf("|----------- Block %p Info Start ---------| \n", bp);
    printf("|   Header: <size-%d> <prev_alloc-%x> <alloc-%x> |\n",
            GET_BLK_SIZE(bp), GET_BLK_PREVBLK_ALLOC(bp), GET_BLK_ALLOC(bp));

    printf("|   Footer: <size-%d> <prev_alloc-%x> <alloc-%x> |\n",
            GET_SIZE(FTRP(bp)), GET_PREVBLK_ALLOC(FTRP(bp)), GET_ALLOC(FTRP(bp)));

    printf("|----------- Block %p Info End -----------| \n", bp);
}

static inline void print_epilogue_block(void) {
    printf("+++++++ Let's print the Epilogue Block +++++++ \n");

    // Bptr bp = (Bptr *)((char *)(mem_heap_hi()));
    Bptr bp = (Bptr)((char *)(mem_heap_hi()) - WSIZE + 1);

    printf("|----------- Block %p Info Start ---------| \n", bp);
    printf("|   Header: <size-%x> <prev_alloc-%x> <alloc-%x> |\n",
            GET_SIZE(bp), GET_PREVBLK_ALLOC(bp), GET_ALLOC(bp));
    printf("|----------- Block %p Info End -----------| \n", bp);


    // printf("+++++++ Print the Epilogue Block End  +++++++ \n");
}

static inline void print_seglists(void) {
    printf(" +++++++ Let's print Seglists +++++++ \n");

    for (int c = 0; c < SIZE_SEGLIST; c++) {
        PRINT_seglist_by_class(c);
    }

    printf("+++++++ Print Seglists End  +++++++ \n");
}



static inline void print_seglist_by_class(int bClass) {

    printf("|+++++++ Let's print Seglist of class %d +++++++ |\n", bClass);

    printf("|   Seglist %p, bClass %d:              | \n", seg_listp + bClass, bClass);
    unsigned int *bp_offset = get_seglist_by_bClass(bClass);
    printf("|   Seglist start offset: %x                    |\n", *bp_offset);

    Bptr bp = int2pointer(*bp_offset);

    for (; bp != NULL; bp = int2pointer(*NEXT_FREE_BLK_OFFSET_P(bp))) {
        dbg_printf("-------------------------------------------------\n");

        dbg_printf("|              free block %p              |\n", bp);
        PRINT_free_block(bp);
        CHECKBLOCK(bp);
        dbg_printf("-------------------------------------------------\n");

    }

    printf("|++++++++++ Print Seglist of class %d END +++++++++++| \n", bClass);

}


static inline void print_block(Bptr bp) {
    // printf("+++++++ Let's print Block +++++++ \n");

    printf("|----------- Block %p Info Start ---------| \n", bp);
    printf("|   Header: <size-%d> <prev_alloc-%x> <alloc-%x> |\n",
            GET_BLK_SIZE(bp), GET_BLK_PREVBLK_ALLOC(bp), GET_BLK_ALLOC(bp));

    printf("|   Footer: <size-%d> <prev_alloc-%x> <alloc-%x> |\n",
            GET_SIZE(FTRP(bp)), GET_PREVBLK_ALLOC(FTRP(bp)), GET_ALLOC(FTRP(bp)));

    printf("|----------- Block %p Info End -----------| \n", bp);
}



static inline void print_free_block(Bptr bp) {
    // printf("+++++++ Let's print Block +++++++ \n");

    printf("|----------- Free Block %p Info Start ---------| \n", bp);
    printf("|   Header: <size-%d> <prev_alloc-%x> <alloc-%x> |\n",
            GET_BLK_SIZE(bp), GET_BLK_PREVBLK_ALLOC(bp), GET_BLK_ALLOC(bp));

    printf("|   NextB offset: %x                            |\n",
                *NEXT_FREE_BLK_OFFSET_P(bp));

    printf("|   PrevB offset: %x                            |\n",
                *PREV_FREE_BLK_OFFSET_P(bp));

    printf("|   Footer: <size-%d> <prev_alloc-%x> <alloc-%x> |\n",
            GET_SIZE(FTRP(bp)), GET_PREVBLK_ALLOC(FTRP(bp)), GET_ALLOC(FTRP(bp)));

    printf("|----------- Free Block Block %p Info End -----------| \n", bp);
}



/***************************
 ***** Helper functions ****
 ***************************/

/*
* Return whether the pointer is in the heap.
* May be useful for debugging.
*/
static int in_heap(const Bptr p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
* Return whether the pointer is aligned.
* May be useful for debugging.
*/
static int aligned(const Bptr p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * int2pointer:
 *      convert 4 byte int offset to 8 byte address pointer
 *      when printing heap_listp in mm_init(), we can find
 *      that the start address is 0x800000000
 */
static inline Bptr int2pointer(unsigned int offset) {

    if (offset) {
        return (Bptr)((long)offset + 0x800000000UL);
    }
    else {

        #ifdef DEBUG
        printf("int2pointer: offset is 0. no block in this class\n");
        // exit(-1);
        #endif

        return NULL;
    }

}

/*
 * pointer2int:
 *      convert 8 byte address pointer to 4 byte int offset
 *      when printing heap_listp in mm_init(), we can find
 *      that the start address is 0x800000000
 */
static inline unsigned int pointer2int(Bptr p) {
    if (p != NULL) {
        return (unsigned int)((long)p - 0x800000000UL);
    }
    else {

        #ifdef DEBUG
        printf("pointer2int: Bptr is null.\n");
        // exit(-1);
        #endif

        return INT_INIT;
    }
}

