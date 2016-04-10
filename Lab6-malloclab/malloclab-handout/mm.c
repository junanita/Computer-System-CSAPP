/*
* mm.c
*
* Name: Ningna Wang
* AndrewID: ningnaw
*
* This solution was implemented using Segregated Free List + Fist fit
* (For implicit free list, please checkout mm-textbook.c)
*
************************************************************************
*
* Free & Allocated Block:
*       each block has at least 4 elements and minimum 16 bytes
*
* since the maximun heap size is 2^32 bytes, so we can use 4 byte
* to store the prev/next block address offset. And we know that the start
* address of the heap is 0x800000000, so we can calculate and store
* offset in our block to save space.
*
* also use the last bit as allocated flag (1: allocated, 0: free)
*
* 1. header (4 bytes): block size + alloc flag
* 2. prev block offset  (4 bytes)
* 3. next block offset  (4 bytes)
* 4. footer (4 bytes): block size + alloc flag
*
*  ------- Free block ---------
*
*   31 30 .... 3 2 1    0
*    ----------------------
*   |    bSize       |alloc|    <- free block footer (4 bytes)
*   |----------------------|
*   |                      |
*   |       unused         |
*   |                      |
*   |----------------------|
*   |prev free block offset|    <- (4 bytes)
*   |----------------------|
*   |next free block offset|    <- (4 bytes)
*   |----------------------|
*   |    bSize       |alloc|    <- free block header (4 bytes)
*   |----------------------|
*
*  ------ Allocated block -------
*
*   31 30 .... 3 2 1    0
*    ----------------------
*   |    bSize       |alloc|    <- allocated block footer (4 bytes)
*   |----------------------|
*   |                      |
*   | payload and padding  |
*   |                      |
*   |----------------------|
*   |    bSize       |alloc|    <- allocated block header (4 bytes)
*   |----------------------|
*
***********************************************************************
*
* Seglists:
*       seglists are seperated by block class
*       the seglists are placed at the head of heap, right after the
*       prologue header.
*
*  ---------- Seglist Instruction ---------
*      size             bSize:      bClass:
*  (=bSize/DSIZE)
*        < 3             < 24         0
*        < 4             < 32         1
*        < 4             < 32         2
*        < 8             < 64         3
*        < 10            < 80         4
*        < 12            < 96         5
*        < 14            < 112        6
*        < 16            < 128        7
*        < 32            < 256        8
*        < 64            < 512        9
*        < 128           < 1024       10
*        < 512           < 4096       11
*        < 1024          < 8192       12
*        < 2048          < 16384      13
*        >= 1048         >=16384      14
*
************************************************************************
*
* Heap alignment:
*                             Heap End
*                     |                      |
*                     |                      |
*                     |----------------------|
*                     |    Epilogue header   | <---     (4 bytes)
* --------------------|----------------------|-------------------------
*                     |                      |
*                     |       .....          |
*                     |     more blocks      |
*                      ----------------------
*                     |   block 1 footer     |<----
*                     |----------------------|    |
*                     |                      |    |
*                     |       unused         |    |
*                     |                      |    |
*                     |----------------------|    |----- free block
*                     |prev free block offset|    |
*                     |----------------------|    |
*                     |next free block offset|    |
*                     |----------------------|    |
*                     |   block 1 header     |<----
*                     |----------------------|
*                     |   block 0 footer     |<----
*                     |----------------------|    |
*                     |                      |    |
*                     |      payload         |    |----- allocated block
*     heap_listp ---> |                      |    |
*                     |----------------------|    |
*                     |   block 0 header     |<----
*  -------------------|----------------------|-----------------------
*                     |   seglist 14 footer  |<----
*                     |----------------------|    |
*                     |prev free block offset|    |
*                     |----------------------|    |---- seglist class 14
*                     |next free block offset|    |         (16 bytes)
*                     |----------------------|    |
*                     |   seglist 14 header  |<----
*                     |----------------------|
*                     |                      |
*                     |                      |
*                     |       ....           |
*                     |                      |
*                     |----------------------|
*                     |   seglist 0 footer   |<----
*                     |----------------------|    |
*                     |prev free block offset|    |
*                     |----------------------|    |---- seglist class 0
*     seg_listp ----> |next free block offset|    |         (16 bytes)
*                     |----------------------|    |
*                     |   seglist 0 header   |<----
*  -------------------|----------------------|-------------------------
*                     |   Prologue header    |    <- (4 bytes)
*                     |----------------------|
*
*                            Heap Start
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
# define PRINT_seglist_by_class(bClass) print_seglist_by_class(bClass)
# define PRINT_block(bp) print_block(bp)
#else
# define dbg_printf(...)
# define CHECKHEAP(lineno)
# define CHECKBLOCK(bp)
# define PRINT_seglist_by_class(bClass)
# define PRINT_block(bp)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */


/**********************************************************************
 ************************* For Basic Definations **********************
 **********************************************************************/

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<11)  /* Extend heap by this amount (bytes) */

/* size of seglist (number of roots in seglist) */
#define SIZE_SEGLIST 15

#define MAX(x, y) ((x) > (y)? (x) : (y))


/****************** Minimun block structure (16 bytes)******************
 *
 * store ALLOC flag:
 *      use the last bit as allocated flag (1: allocated, 0: free)
 *
 * store OFFSET:
 *      since the maximun heap size is 2^32 bytes, so we can use 4 byte
 * to store the prev/next block address offset. And we know that the start
 * address of the heap is 0x800000000, so we can calculate and store
 * offset in our block to save space.
 -----------------------------------------------------------------------
    31 30 .... 3 2 1    0
     -----------------------
    |    bSize       |alloc|    <- Footer (4 bytes) -
    |-----------------------                         |
    |prev free block offset|                         |
    |-----------------------                         |-- total 16 bytes
    |next free block offset|                         |
    |-----------------------                         |
    |    bSize       |alloc|    <- Header (4 bytes) -
    |-----------------------
***********************************************************************/

/* mininum block size */
#define MIN_BLK_SIZE 16


/**********************************************************************
 **************** For Both Allocated Blocks and Free Blocks ***********
 **********************************************************************/

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* write header without modify prev alloc flag */
#define PUT_HEAD(p, val) (GET(p) = (GET(p) & 0x2) | (val))

/* write 4 byte pointer offset val to p */
#define PUT_OFFSET(p, val) (*(int *)(p) = (int)(val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, read the size and allocated fields */
#define GET_BLK_SIZE(bp)  (GET(HDRP(bp)) & ~0x7)
#define GET_BLK_ALLOC(bp) (GET(HDRP(bp)) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


/* Given free block bp, return offset pointer represents next/prev block */
#define NEXT_FREE_BLK_OFFSET_P(bp) ((unsigned int *)((char *)(bp)))
#define PREV_FREE_BLK_OFFSET_P(bp) ((unsigned int *)((char *)(bp) + WSIZE))


/***********************************************************************
 ************************ Global variables *****************************
 ***********************************************************************/

/* put seglists at the beginning of the heap */
static char *seg_listp = 0; // point to first block of seglists on heap

/* Pointer to first block after seglists */
static char *heap_listp = 0;

/* block pointer type: (void *) */
typedef void *Bptr;


/***********************************************************************
 ************ Function prototypes for internal helper routines *********
 ***********************************************************************/

static Bptr extend_heap(size_t words);
static void place(Bptr bp, size_t asize);
static Bptr find_fit(size_t asize);
static Bptr coalesce(Bptr bp);
static inline void seglists_init(void);
static inline Bptr get_seglist_by_bClass(int bClass);
static inline unsigned int get_bClass_by_bSize(size_t bSize);
static inline void remove_fblock_from_seglist(Bptr bp);
static inline void add_fblock_to_seglist(Bptr bp, size_t bSize);
static inline Bptr get_next_fblck_p(Bptr bp);
static inline Bptr get_prev_fblck_p(Bptr bp);


/***********************************************************************
 ************************* Helper functions ****************************
 ***********************************************************************/

static inline Bptr int2pointer(unsigned int offset);
static inline unsigned int pointer2int(Bptr p);
static inline void print_seglists(void);
static inline void print_seglist_by_class(int bClass);
static inline void print_prologue_header(void);
static inline void print_epilogue_header(void);
static inline void print_block(Bptr bp);
static inline void print_block(Bptr bp);
static inline void check_block(Bptr bp);
static int in_heap(const Bptr p);
static int aligned(const Bptr p);


/***********************************************************************
 ************************* Start Implementation ************************
 ***********************************************************************/

/*
 * mm_init - initialize the memory manager
 *
 * Instructions: put the seglists block at the beginning of heap
 *               as virtual memory
 *
 * Returns:
 *   -1:         error
 *    0:         success.
 */
int mm_init(void) {
    dbg_printf("------- In mm_init function -------\n");

    /* Create the initial empty heap */

    /* create the initial empty seglists blocks*/
    if ((seg_listp = mem_sbrk(ALIGN(MIN_BLK_SIZE* SIZE_SEGLIST)
                                + DSIZE)) == (void *)-1)
        return -1;

    dbg_printf("The start address of heap is %p \n", seg_listp);

    heap_listp = seg_listp;
    PUT(heap_listp, PACK(0, 1));              /* set prologue header */

    /* seg_listp point to first block in seglists*/
    seg_listp += (2 * WSIZE);

    /* heap_listp point to first block after seglists */
    heap_listp += SIZE_SEGLIST * MIN_BLK_SIZE + DSIZE;

    PUT(HDRP(heap_listp), PACK(0, 1));  /* set epilogue header */

    /* Init seglists in heap */
    seglists_init();

    dbg_printf("check heap before extend----- \n");
    CHECKHEAP(1);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    dbg_printf("check heap after extend----\n");
    CHECKHEAP(1);

    dbg_printf("------- In mm_init function END -------\n");
    return 0;
}

/*
 * malloc
 *
 * Instructions: The malloc routine returns a pointer to an allocated
 *               block payload of at least size bytes.
 *
 * Parameters:   size that need to be allocated
 *
 * Returns:       8-byte aligned block pointer
 */
Bptr malloc(size_t size) {
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
    extendsize = MAX(asize, CHUNKSIZE/DSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    dbg_printf("----------------- In malloc function END -----------------\n");

    return bp;
}

/*
 * mm_free - free an allocated block, given block pointer bp
 *
 * Instructions: This routine is only guaranteed to work when the passed
 *               pointer (ptr) was returned by an earlier call to malloc,
 *               calloc, or realloc and has not yet been freed.
 *               free(NULL) has no effect.
 * Parameters:   block pointer that need to be free
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

    size_t size = GET_BLK_SIZE(bp);

    /* check block size */
    if (size < MIN_BLK_SIZE) {
        dbg_printf("Error: block cannot be freed: size not right\n");
        exit(-1);
        return;
    }

    /* Reset header/footer */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    dbg_printf("Let's print block %p after free \n", bp);
    PRINT_block(bp);
    CHECKBLOCK(bp);

    coalesce(bp);

    dbg_printf("--------------- In mm_free function END ---------------\n");
}

/*
 * realloc - returns a pointer to an allocated region of at least size bytes
 *
 * Instructions:  - Case 1: the return address of the new block is the same
 *                          as the old block, place new block
 *                    case 1.1: the new size was smaller than the old size
 *                    case 1.2: there was free space after the old block
 *
 *                - Case 2: the returned address is different than old block
 *                          malloc space for new block
 *
 * Returns:       block pointer after realloc
 */
Bptr realloc(Bptr bp, size_t size) {
    dbg_printf("----------------- In realloc function -------------------\n");

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
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    dbg_printf("size: %zu, oldsize: %zu \n", size, oldsize);

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

        if (asize < bSize) {
            dbg_printf("asize %zu <= extended block size %zu \n", asize, bSize);
            remove_fblock_from_seglist(NEXT_BLKP(bp));

            /* Create block's header/footer */
            PUT(HDRP(bp), PACK(bSize, 1));
            PUT(FTRP(bp), PACK(bSize, 1));

            PRINT_block(bp);
            CHECKBLOCK(bp);

            place(bp, asize);
            dbg_printf("----------------In realloc function END----------\n");

            return bp;
        }
    }

    /* Case 2: the returned address is different than the address passed
                in, the old block has been freed and should not be used,
                freed, or passed to realloc again */

    dbg_printf("Case 2: the returned address is different than old block\n");

    new_bp = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!new_bp) {
        return 0;
    }

    /* Copy the old data. */
    dbg_printf("Copy the old data:\n");
    dbg_printf("old size %zu \n", oldsize);
    dbg_printf("current size %zu \n", size);

    if(size < oldsize) oldsize = size;
    memcpy(new_bp, bp, oldsize);

    /* Free the old block. */
    mm_free(bp);

    dbg_printf("--------------In realloc function END-----------------\n");
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


/**********************************************************************
 *********** Function prototypes for internal helper routines *********
 **********************************************************************/

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 *
 * Instructions:    1. no prev/next free block
 *                  2. has next free block
 *                  3. has prev free block
 *                  4. has both prev & next free block
 *
 * Parameters:      block pointer that need to be coalesced
 *
 * Returns:         block pointer that after coalesced
 */
static Bptr coalesce(Bptr bp) {
    dbg_printf("--------------- In coalesce function ---------------\n");

    /* get prev and next block pointer */
    Bptr prevB = PREV_BLKP(bp);
    Bptr nextB = NEXT_BLKP(bp);

    size_t prev_alloc = GET_ALLOC(FTRP(prevB));
    size_t next_alloc = GET_ALLOC(HDRP(nextB));
    size_t size = GET_SIZE(HDRP(bp));

    dbg_printf("Let's check block %p before coalesce: \n", bp);
    PRINT_block(bp);
    CHECKBLOCK(bp);

    if (prev_alloc && next_alloc) {                     /* Case 1 */
        dbg_printf("Case1: no coalesce \n");
        add_fblock_to_seglist(bp, GET_BLK_SIZE(bp));
        dbg_printf("------------- In coalesce function END------------\n");
        return bp;
    }

    else if (prev_alloc && !next_alloc) {               /* Case 2 */
        dbg_printf("Case2: coalesce next block \n");
        remove_fblock_from_seglist(nextB);

        /* setup new free block */
        size += GET_SIZE(HDRP(nextB));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_fblock_to_seglist(bp, GET_BLK_SIZE(bp));
        return bp;
    }

    else if (!prev_alloc && next_alloc) {               /* Case 3 */
        dbg_printf("Case3: coalesce prev block %p \n", prevB);
        PRINT_block(prevB);
        remove_fblock_from_seglist(prevB);

        /* setup new free block */
        size += GET_SIZE(HDRP(prevB));
        bp = prevB;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_fblock_to_seglist(bp, GET_BLK_SIZE(bp));
        dbg_printf("------------- In coalesce function END------------\n");
        return bp;
    }

    else {                                              /* Case 4 */
        dbg_printf("Case4: coalesce prev + next block \n");
        remove_fblock_from_seglist(nextB);
        remove_fblock_from_seglist(prevB);

        /* setup new free block */
        size += GET_SIZE(HDRP(prevB)) + GET_SIZE(FTRP(nextB));
        bp = prevB;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_fblock_to_seglist(bp, GET_BLK_SIZE(bp));
        dbg_printf("------------- In coalesce function END------------\n");
        return bp;
    }
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 *
 * Instructions:    1. split free block when remainder is
 *                      at least MIN_BLK_SIZE
 *                  2. use all of the free block
 *
 * Note:    need to consider realloc blocks which are not in seglists
 *
 * Parameters:
 *      bp:         block pointer that used to place
 *      asize:      size that need to be placed
 */
static void place(Bptr bp, size_t asize) {
    dbg_printf("----------------- In place function -----------------\n");

    size_t csize = GET_SIZE(HDRP(bp));
    size_t diff = csize - asize;
    /* handle realloc block */
    int realloc_flag = GET_BLK_ALLOC(bp);

    dbg_printf("block size: %zu \n", csize);
    dbg_printf("content size: %zu \n", asize);
    dbg_printf("block pointer: %p \n", bp);
    dbg_printf("realloc_flag: %d \n", realloc_flag);

    /* Case 1: split when remainder is at least MIN_BLK_SIZE */
    if (diff >= MIN_BLK_SIZE) {

        dbg_printf("remainder %zu >= MIN_BLK_SIZE, Split! \n", diff);
        if (!realloc_flag) {
            remove_fblock_from_seglist(bp);
        }

        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));


        dbg_printf("========= print allocated block ========\n");
        PRINT_block(bp);
        CHECKBLOCK(bp);

        /* setup new free block */
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(diff, 0));
        PUT(FTRP(bp), PACK(diff, 0));


        dbg_printf("========= print new free block ========\n");
        PRINT_block(bp);
        CHECKBLOCK(bp);

        add_fblock_to_seglist(bp, GET_BLK_SIZE(bp));
    }

    /* Case 2: Use all of the free block */
    else {
        dbg_printf("remainder %zu < MIN_BLK_SIZE, Use all of it! \n", diff);

        if (!realloc_flag) {
            remove_fblock_from_seglist(bp);
        }

        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        dbg_printf("========= print free block after place ========\n");
        PRINT_block(bp);
        CHECKBLOCK(bp);
    }

    dbg_printf("----------------- In place function END-----------------\n");
}

/*
 * find_fit - Find a fit for a block with asize bytes,
 *
 * Instructions: In each seglist, free blocks are arranged based on block
 *               size, in ascending order
 *               I use first-fit to find the fit block in each seglist
 *
 * Parameters:   size that used to find fit free block
 *
 * Returns:
 *   Bptr:       block pointer that find the fit free block
 *   NULL:       no fit free block found
 */
static Bptr find_fit(size_t asize) {
    dbg_printf("---------------- In find_fit function ------------- \n");
    Bptr seglist;
    int bClass = get_bClass_by_bSize(asize);

    /* search seglists */
    for (int c = bClass; c < SIZE_SEGLIST; c++) {

        seglist = get_seglist_by_bClass(c);
        Bptr bp = get_next_fblck_p(seglist);

        /* find the fit free block */
        while (bp != NULL) {
            if (asize <= GET_BLK_SIZE(bp)) {
                dbg_printf("Find the fit block %p, size %d \n",
                            bp, GET_BLK_SIZE(bp));
                return bp;
            }
            /* check next block */
            bp = get_next_fblck_p(bp);
        }

        dbg_printf("cannot find fit block of class %d, go to next class \n",
                    c);
    }

    /* not find the fit block in fit class */
    dbg_printf("cannot find the fit block \n");

    dbg_printf("---------------- In find_fit function END ------------- \n");
    return NULL;
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 *
 * Parameters:   extended size
 *
 * Returns:
 *   Bptr:       block pointer after extend the heap
 *   NULL:       cannot extend heap anymore
 */
static Bptr extend_heap(size_t words) {
    dbg_printf("---------------- In extend_heap function ------------- \n");

    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (void *) -1)
        return NULL;

    dbg_printf("Extended the %p by %zu size.\n", bp, size);

    /* Initialize free block header/footer */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */

    /* Initialize the epilogue header */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    dbg_printf("Let's check extended block %p \n", bp);
    PRINT_block(bp);
    CHECKBLOCK(bp);

    /* Coalesce if the previous block was free */
    Bptr new_bp = coalesce(bp);
    dbg_printf("------------- In extend_heap function END ----------- \n");
    return new_bp;
}

/*
 * add_fblock_to_seglist - add free block to corresponding seglist by
 *                          by block size
 *
 * Instructions:  each seglist is arranged by block size, in ascending order
 *
 * Parameters:
 *      bp:       block pointer point to block that need to be added
 *      bSize:    block size
 */
static inline void add_fblock_to_seglist(Bptr bp, size_t bSize) {
    dbg_printf("------- In add_fblock_to_seglist function ---------- \n");

    int bClass = get_bClass_by_bSize(bSize);
    Bptr seglist = get_seglist_by_bClass(bClass);

    dbg_printf("Add new free block %p to seglist %p \n", bp, seglist);
    dbg_printf("Let's print the seglist %p before adding: %p \n",
                seglist, bp);
    PRINT_seglist_by_class(bClass);

    Bptr next_bp = get_next_fblck_p(seglist);
    Bptr prev_bp = seglist;

    /* find the fit position to add, block size in ascending order */
    while (next_bp != NULL && GET_SIZE(HDRP(next_bp)) < bSize) {
        prev_bp = next_bp;
        next_bp = get_next_fblck_p(next_bp);
    }
    dbg_printf("Find the right way to put the new free block \n");

    /* add block to seglist */
    PUT_OFFSET(NEXT_FREE_BLK_OFFSET_P(bp), pointer2int(next_bp));
    PUT_OFFSET(PREV_FREE_BLK_OFFSET_P(bp), pointer2int(prev_bp));
    PUT_OFFSET(NEXT_FREE_BLK_OFFSET_P(prev_bp), pointer2int(bp));

    if (next_bp != NULL) {
        PUT_OFFSET(PREV_FREE_BLK_OFFSET_P(next_bp), pointer2int(bp));
    }

    dbg_printf("Let's print the seglist %p after adding: %p \n",
                seglist, bp);
    PRINT_seglist_by_class(bClass);
    dbg_printf("------- In add_fblock_to_seglist function END---------- \n");

}

/*
 * remove_fblock_from_seglist - remove free block from corresponding
 *                              seglist by block size
 *
 * Parameters:
 *      bp:       block pointer point to block that need to be removed
 */
static inline void remove_fblock_from_seglist(Bptr bp) {
    dbg_printf("------- In remove_fblock_from_seglist function -------\n");

    #ifdef DEBUG
        int bClass = get_bClass_by_bSize(GET_BLK_SIZE(bp));
        Bptr seglist = get_seglist_by_bClass(bClass);
    #endif
    dbg_printf("Remove free block %p from seglist %p \n", bp, seglist);
    PRINT_block(bp);
    dbg_printf("Let's print the seglist %p before removing: %p \n",
                seglist, bp);
    PRINT_seglist_by_class(bClass);

    /* get prev/next block pointer */
    Bptr prev_bp = get_prev_fblck_p(bp);
    Bptr next_bp = get_next_fblck_p(bp);

    /* update block info */
    PUT_OFFSET(NEXT_FREE_BLK_OFFSET_P(prev_bp), pointer2int(next_bp));

    if (next_bp != NULL) {
        dbg_printf("next_bp exist \n");
        PUT_OFFSET(PREV_FREE_BLK_OFFSET_P(next_bp), pointer2int(prev_bp));
    }

    dbg_printf("Let's print the seglist %p after removing: %p \n", seglist, bp);
    PRINT_seglist_by_class(bClass);
    dbg_printf("------- In remove_fblock_from_seglist function END -------\n");
}

/*
 * seglists_init - initialize the seglists at the head of heap
 *
 * Note:    call by mm_init() function
 */
static inline void seglists_init(void) {

    for (int bClass = 0; bClass < SIZE_SEGLIST; bClass++) {

        char *seglist = (char *)get_seglist_by_bClass(bClass);

        /* seglist header */
        PUT(seglist - WSIZE, PACK(MIN_BLK_SIZE, 1));

        /* seglist next free block */
        PUT_OFFSET(seglist, pointer2int(NULL));

        /* seglist prev free block */
        PUT_OFFSET(seglist + (1*WSIZE), pointer2int(NULL));

        /* seglist footer */
        PUT(seglist + (2*WSIZE), PACK(MIN_BLK_SIZE, 1));
    }
}

/*
 * get_seglist_by_bClass - get seglist block pointer by block class
 *
 * Note:        seg_listp is the start of seglists block pointer
 *
 * Parameters:  block class
 *
 * Returns:     block pointer that point to seglist block
 */
static inline Bptr get_seglist_by_bClass(int bClass) {

    if (bClass != -1) {
        return seg_listp + bClass * MIN_BLK_SIZE;
    }
    else {
        #ifdef DEBUG
            printf("----- Error: bClass is < 0 -----\n");
        #endif
        return NULL;
    }
}

/*
 * get_bClass_by_bSize - Get block class by block size
 *
 * Usage:          using for index Seglists
 * Instruction:    many different lists for small(ish) block sizes,
 *                 one class for each power of two for larger sizes
 * Parameters:
 *   bSize:         block size
 *
 * Returns:
 *   0~14:          block class
 *
 *   ********* Seglist Instruction *********
 *      size             bSize:      bClass:
 *  (=bSize/DSIZE)
 *        < 3             < 24         0
 *        < 4             < 32         1
 *        < 4             < 32         2
 *        < 8             < 64         3
 *        < 10            < 80         4
 *        < 12            < 96         5
 *        < 14            < 112        6
 *        < 16            < 128        7
 *        < 32            < 256        8
 *        < 64            < 512        9
 *        < 128           < 1024       10
 *        < 512           < 4096       11
 *        < 1024          < 8192       12
 *        < 2048          < 16384      13
 *        >= 1048         >=16384      14
 */
static inline unsigned int get_bClass_by_bSize(size_t bSize) {

    if (bSize < MIN_BLK_SIZE) {
        dbg_printf("Error: Your bSize %zu is smaller than MIN_BLK_SIZE\n",
                    bSize);
        exit(-1);
    }

    int size = bSize / DSIZE;
    int bClass;

    dbg_printf("block with bSize %zu is size %d \n", bSize, size);

    /* Select class(index) of Seglist */
    if (size < 3) bClass = 0;
    else if (size < 4) bClass = 1;
    else if (size < 6) bClass = 2;
    else if (size < 8) bClass = 3;
    else if (size < 10) bClass = 4;
    else if (size < 12) bClass = 5;
    else if (size < 14) bClass = 6;
    else if (size < 16) bClass = 7;
    else if (size < 32) bClass = 8;
    else if (size < 64) bClass = 9;
    else if (size < 128) bClass = 10;
    else if (size < 512) bClass = 11;
    else if (size < 1024) bClass = 12;
    else if (size < 2048) bClass = 13;
    else bClass = 14;

    dbg_printf("Block with bSize %zu is class %d \n", bSize, bClass);
    return bClass;
}

/*
 * get_next_fblck_p - get next free block pointer
 *
 * Parameters:
 *  Bptr bp:           block pointer
 *
 * Returns:
 *   Bptr:             block pointer
 */
static inline Bptr get_next_fblck_p(Bptr bp) {
    unsigned int *offset = NEXT_FREE_BLK_OFFSET_P(bp);
    return int2pointer(*offset);
}

/*
 * get_prev_fblck_p - get previous free block pointer
 *
 * Parameters:
 *  Bptr bp:           block pointer
 *
 * Returns:
 *   Bptr:             block pointer
 */
static inline Bptr get_prev_fblck_p(Bptr bp) {
    unsigned int *offset = PREV_FREE_BLK_OFFSET_P(bp);
    return int2pointer(*offset);
}


/**********************************************************************
 ************************ Check & Print functions *********************
 **********************************************************************/

/*
 * mm_checkheap - check heap
 *
 * Instructions:    1. check prologue block
 *                  2. check seglists
 *                  3. check all block after seglists
 *                  4. check epilogue block
 *                  5. check heap boundaries
 * Parameters:
 *     0:           not check heap
 *     1:           check heap
 */
void mm_checkheap(int lineno) {

    if (!lineno) {
        printf("Not check the heap. \n");
        return;
    }

    printf("**************** Let's check the Heap **************** \n");

    /* print prologue block */
    print_prologue_header();

    /* check prologue block */
    if (GET_SIZE(seg_listp - DSIZE) != 0 || 
        !GET_ALLOC(seg_listp - DSIZE)) {
        printf("Error: the prologue block has some problems \n");
        printf("GET_SIZE(seg_listp - DSIZE): %x, Correct size: %x \n",
               GET_SIZE(seg_listp - DSIZE), 0);
        printf("GET_ALLOC(seg_listp - DSIZE): %x \n", 
                GET_ALLOC(seg_listp - DSIZE));
        exit(-1);
    }

    /* print & check seglists which are in the head of heap */
    print_seglists();

    /* print & check each block (after seglists) */
    char *tmp_ptr = heap_listp;
    for (; GET_BLK_SIZE(tmp_ptr) > 0; tmp_ptr = NEXT_BLKP(tmp_ptr)) {
        PRINT_block(tmp_ptr);
        CHECKBLOCK(tmp_ptr);
    }

    /* print epilogue block */
    print_epilogue_header();

    /* check epilogue block */
    if (GET_BLK_SIZE(tmp_ptr) != 0 || !GET_BLK_ALLOC(tmp_ptr)) {
        printf("Error: the epilogue block has some problems \n");
        printf("GET_BLK_SIZE(tmp_ptr): %x, correct size: %x \n",
                GET_BLK_SIZE(tmp_ptr), 0);
        printf("GET_BLK_ALLOC(tmp_ptr): %x \n", GET_BLK_ALLOC(tmp_ptr));

        exit(-1);
    }

    /* check heap boundaries. */
    if (seg_listp - DSIZE != mem_heap_lo()) {
        printf("Error: your start pointer %p does not match heap_lo %p\n",
                seg_listp - DSIZE, mem_heap_lo());
        exit(-1);
    }
    if (tmp_ptr - 1 != mem_heap_hi()) {
        printf("Error: your end pointer %p does not match heap_lo %p\n",
                tmp_ptr - 1, mem_heap_hi());
        exit(-1);
    }

    printf("**************** Check the Heap End ****************** \n");
}

/*
 * check_block - Check block
 *
 * Instructions:    1. check block's address alignment
 *                  2. check block's header and footer
 *                  3. check block's size
 *                  4. check if block exceed heap boundaries
 *
 * Parameters:
 *      bp:         block pointer
 */
static inline void check_block(Bptr bp) {

    printf("=== Let's check block %p ====  \n", bp);

    /* Check each block’s address alignment. */
    if (!aligned(bp)) {
        printf("The free block %p is not aligned\n", bp);
        exit(-1);
    }

    /* Check each if block's header and footer matching each other */
    if (GET(HDRP(bp)) != GET(FTRP(bp))) {
        printf("Block header %x does not match footer %x \n",
                GET(HDRP(bp)), GET(FTRP(bp)));
        exit(-1);
    }

    /* check block minimum size */
    if (GET_BLK_SIZE(bp) < MIN_BLK_SIZE) {
        printf("Block size %x is smaller than minimum size %d \n",
                GET_BLK_SIZE(bp), MIN_BLK_SIZE);
        exit(-1);
    }

    /* check if block exceed heap boundaries. */
    if ((Bptr)HDRP(bp) <= mem_heap_lo()) {
        printf("Error: your block header %p is lower than match heap_lo %p\n",
                HDRP(bp), mem_heap_lo());
        exit(-1);
    }
    if ((Bptr)FTRP(bp) >= mem_heap_hi()) {
        printf("Error: your block footer %p is higher than match heap_lo %p\n",
                FTRP(bp), mem_heap_hi());
        exit(-1);
    }

    printf("====  Check free block %p End ==== \n", bp);

}

/*
 * print_prologue_header - print prologue header
 *
 * Note:            seg_listp is start pointer of seglists, which are
 *                  right behand the prologue header
 */
static inline void print_prologue_header(void) {

    Bptr bp = seg_listp - DSIZE;

    printf("|----------- Prologue %p Info Start ---------| \n", bp);
    printf("|   Header: <size-%d>   <alloc-%x>                      |\n",
            GET_SIZE(bp),  GET_ALLOC(bp));

    printf("|----------- Prologue %p Info End -----------| \n", bp);
}

/*
 * print_epilogue_header - print epilogue header
 *
 * Note:            mem_heap_hi() is the end heap
 */
static inline void print_epilogue_header(void) {

    Bptr bp = (Bptr)((char *)(mem_heap_hi()) - WSIZE + 1);

    printf("|----------- Epilogue %p Info Start ---------| \n", bp);
    printf("|   Header: <size-%x>    <alloc-%x>                     |\n",
            GET_SIZE(bp), GET_ALLOC(bp));
    printf("|----------- Epilogue %p Info End -----------| \n", bp);

}


/*
 * print_seglists - print seglists and each block in seglist
 *
 * Note: call print_seglist_by_class() function
 */
static inline void print_seglists(void) {
    printf(" +++++++++++++ Let's print Seglists +++++++++++++++ \n");
    for (int c = 0; c < SIZE_SEGLIST; c++) {
        PRINT_seglist_by_class(c);
    }
    printf("++++++++++++++ Print Seglists End  ++++++++++++++ \n");
}

/*
 * print_seglist_by_class - print & check a specific seglist
 *                          given block class
 * Parameters:      block class
 */
static inline void print_seglist_by_class(int bClass) {
    printf("|+++++++ Let's print Seglist of class %d +++++++ |\n", bClass);

    Bptr seglist = get_seglist_by_bClass(bClass);

    printf("-------------------------------------------------\n");
    printf("|   Seglist %p, bClass %d:              | \n", seglist, bClass);

    Bptr bp = seglist;

    for (; bp != NULL; bp = int2pointer(*NEXT_FREE_BLK_OFFSET_P(bp))) {
        printf("-------------------------------------------------\n");
        PRINT_block(bp);
        CHECKBLOCK(bp);
        printf("-------------------------------------------------\n");
    }

    printf("|++++++++++ Print Seglist of class %d END +++++++++++| \n",
            bClass);
}

/*
 * print_block - print block info given block pointer
 *
 * Parameters:   block pointer
 */
static inline void print_block(Bptr bp) {

    printf("|-------- Block %p Info Start --------| \n", bp);
    printf("|   Header: <size-%d>   <alloc-%x>              |\n",
            GET_BLK_SIZE(bp), GET_BLK_ALLOC(bp));

    printf("|   NextB offset: %x                            |\n",
                *NEXT_FREE_BLK_OFFSET_P(bp));

    printf("|   PrevB offset: %x                            |\n",
                *PREV_FREE_BLK_OFFSET_P(bp));

    printf("|   Footer: <size-%d>   <alloc-%x>              |\n",
            GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)));

    printf("|-------- Block %p Info End ----------| \n", bp);
}



/***********************************************************************
 *************************** Helper functions **************************
 **********************************************************************/

/*
 * int2pointer - convert 4 byte int offset to 8 byte address pointer
 *
 * Note:    when printing heap_listp in mm_init(), we can find
 *          that the start address is 0x800000000
 */
static inline Bptr int2pointer(unsigned int offset) {

    return offset ? (Bptr)((long)offset | 0x800000000UL) : NULL;
}

/*
 * pointer2int - convert 8 byte address pointer to 4 byte int offset
 *
 * Note:    when printing heap_listp in mm_init(), we can find
 *          that the start address is 0x800000000
 */
static inline unsigned int pointer2int(Bptr p) {

    return p != NULL ? (unsigned int)((long)p - 0x800000000UL) : 0U;
}

/*
 * in_heap - Return whether the pointer is in the heap.
 *           May be useful for debugging.
 */
static int in_heap(const Bptr p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * aligned - Return whether the pointer is aligned.
 *           May be useful for debugging.
 */
static int aligned(const Bptr p) {
    return (size_t)ALIGN(p) == (size_t)p;
}
