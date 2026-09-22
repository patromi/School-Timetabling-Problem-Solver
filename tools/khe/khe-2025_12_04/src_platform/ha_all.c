
/*****************************************************************************/
/*                                                                           */
/*  THE HOWARD OBJECT-ORIENTED COMPILER TOOLKIT                              */
/*  COPYRIGHT (C) 2011 Jeffrey H. Kingston                                   */
/*                                                                           */
/*  Jeffrey H. Kingston (jeff@it.usyd.edu.au)                                */
/*  School of Information Technologies                                       */
/*  The University of Sydney 2006                                            */
/*  AUSTRALIA                                                                */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either Version 3, or (at your option)      */
/*  any later version.                                                       */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program; if not, write to Free Software Foundation       */
/*  Inc., 59 Temple Place, Suite 330, Boston MA 02111-1307 USA               */
/*                                                                           */
/*  FILE:     ha_arena.c                                                     */
/*  PURPOSE:  Memory arenas, arena sets, and extensible arrays               */
/*                                                                           */
/*****************************************************************************/
#include "howard_a.h"
#include "howard_n.h"
#include <stdarg.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0

#define HA_MAX_INDEX	40		/* max block size 2 ^ HA_MAX_INDEX */

#define CHECK 1
#define ARENA_CHECK_MAGIC		1974620
#define ARENA_SET_CHECK_MAGIC		1994620
#define CHUNK_CHECK_MAGIC		2994620
#define FREE_BLOCK_LIST_CHECK_MAGIC	3722285
#define FREE_BLOCK_CHECK_MAGIC		3886541
#define BUSY_BLOCK_CHECK_MAGIC		3876241


/*****************************************************************************/
/*                                                                           */
/*  Some definitions - you'd better read this!                               */
/*                                                                           */
/*  A *byte* is the usual eight-bit piece of memory (the smallest amount     */
/*  with its own memory address), for which sizeof() returns 1.              */
/*                                                                           */
/*  A *word* is sizeof(HA_ALIGN_TYPE) contiguous bytes aligned on a          */
/*  HA_ALIGN_TYPE boundary.  It is the smallest amount of memory worth       */
/*  allocating, given that alignment with HA_ALIGN_TYPE is required.         */
/*  Ha's internal calculations are all done in words, not bytes.             */
/*                                                                           */
/*  A *block* is one or more contiguous words aligned on a HA_ALIGN_TYPE     */
/*  boundary.  It (or its mem[] field in the case of resizable blocks) is    */
/*  passed to the caller in response to a request for memory.  It contains   */
/*  enough memory to satisfy the caller's request, and possibly more.        */
/*                                                                           */
/*  A *resizable block* is a block that can be resized.  In Ha, some blocks  */
/*  are resizable, and some are not resizable.  Errors will occur if an      */
/*  attempt is made to resize an unresizable block.                          */
/*                                                                           */
/*  A *chunk* is one or more contiguous words aligned on a HA_ALIGN_TYPE     */
/*  boundary, received by Ha from malloc, then gradually divided into        */
/*  blocks and passed to callers in response to their requests for memory.   */
/*                                                                           */
/*  A chunk or block is *free* if none of its memory is currently on loan    */
/*  to a caller.  Otherwise the chunk or block is *busy*.                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  MALLOC_OVERHEAD_WORDS                                                    */
/*                                                                           */
/*    The usual amount (in words) of overhead memory created by one call     */
/*    to malloc.  Overhead memory is memory, not usable by the caller,       */
/*    lying just before the start of the usable memory and/or just after     */
/*    the end of the usable memory.                                          */
/*                                                                           */
/*  MALLOC_MIN_CHUNK_WORDS                                                   */
/*                                                                           */
/*    The minimum size (in words) of a chunk returned by malloc, including   */
/*    overhead memory.                                                       */
/*                                                                           */
/*  For example, the smallest amount of memory that it is worth asking       */
/*  malloc for is MALLOC_MIN_CHUNK_WORDS - MALLOC_OVERHEAD_WORDS words.  If  */
/*  you ask for less you still get a chunk of MALLOC_MIN_CHUNK_WORDS words   */
/*  from which MALLOC_OVERHEAD_WORDS words of overhead have been taken.      */
/*                                                                           */
/*  Obviously, these values may differ from one implementation of malloc to  */
/*  another.  The values just below are ones I have observed when testing    */
/*  malloc on my own 64-bit system (see "khe -m").                           */
/*                                                                           */
/*  Ha's non-resizable blocks have no overhead, and its resizable ones have  */
/*  one word of overhead.  It's disappointing that malloc's overhead per     */
/*  chunk should be two words.  However, we take few chunks from malloc.     */
/*                                                                           */
/*****************************************************************************/

#define MALLOC_OVERHEAD_WORDS		 2
#define MALLOC_MIN_CHUNK_WORDS		 4


/*****************************************************************************/
/*                                                                           */
/*  HA_MIN_CHUNK_WORDS                                                       */
/*                                                                           */
/*    The number of words in the smallest chunk requested by Ha from         */
/*    malloc.  This does not include the MALLOC_OVERHEAD_WORDS words of      */
/*    overhead added by malloc, but when you do add this overhead you must   */
/*    get a power of 2.  Actually this value is unused; instead, we use      */
/*    HA_MIN_CHUNK_INDEX, which has the same information in another form.    */
/*                                                                           */
/*  HA_MIN_CHUNK_INDEX                                                       */
/*                                                                           */
/*    The x such that 2^x = HA_MIN_CHUNK_WORDS + MALLOC_OVERHEAD_WORDS       */
/*                                                                           */
/*  HA_INIT_RESIZABLE_BLOCK_WORDS                                            */
/*                                                                           */
/*    The number of words in the smallest resizable block requested by Ha,   */
/*    other than 0.  This value is unaffected by anything malloc does.       */
/*    The value 5 is chosen because when you repeatedly double it and add    */
/*    1 you get a sequence of numbers that make good hash table sizes:       */
/*                                                                           */
/*      5, 11, 23, 47, 95, 191, 383, 767, ...                                */
/*                                                                           */
/*****************************************************************************/

#define HA_MIN_CHUNK_WORDS		14
#define HA_MIN_CHUNK_INDEX		 4
#define HA_INIT_RESIZABLE_BLOCK_WORDS	 5


/*****************************************************************************/
/*                                                                           */
/*  Private typedefs                                                         */
/*                                                                           */
/*****************************************************************************/

typedef struct ha_chunk_rec			*HA_CHUNK;
typedef struct ha_free_resizable_block_list_rec	*HA_FREE_RESIZABLE_BLOCK_LIST;
typedef struct ha_free_resizable_block_rec	*HA_FREE_RESIZABLE_BLOCK;
typedef struct ha_busy_resizable_block_rec	*HA_BUSY_RESIZABLE_BLOCK;


/*****************************************************************************/
/*                                                                           */
/*  HA_CHUNK - a chunk of memory obtained from malloc                        */
/*                                                                           */
/*  next_chunk                                                               */
/*    The next chunk in a singly linked list of chunks, which will be a      */
/*    free list if chunk is free, or a busy list if chunk is busy.           */
/*                                                                           */
/*  mem_total_words                                                          */
/*    The total number of words initially available for allocation to        */
/*    users of Ha in this chunk.  This memory lies in the mem[] field.       */
/*                                                                           */
/*  mem_avail_words                                                          */
/*    The total number of words currently available for allocation to        */
/*    users of Ha in this chunk.  This is initially mem_total_words;         */
/*    it decreases as memory is allocated to users.                          */
/*                                                                           */
/*  index                                                                    */
/*    The index of this chunk, determining how large it is (see below).      */
/*                                                                           */
/*  magic                                                                    */
/*    A magic number that proves that the chunk really is a chunk.           */
/*                                                                           */
/*  mem[1]                                                                   */
/*    A varying-length field holding the memory available to users of Ha.    */
/*                                                                           */
/*  A chunk is obtained by requesting 2^index - MALLOC_OVERHEAD_WORDS        */
/*  words of memory from calloc, for some value of index which is at         */
/*  least HA_MIN_CHUNK_INDEX.  This value is chosen because it means         */
/*  that malloc itself has to allocate a chunk whose size (including         */
/*  malloc's own overhead) is a power of 2.                                  */
/*                                                                           */
/*  It follows that mem[] contains                                           */
/*                                                                           */
/*    2^index - MALLOC_OVERHEAD_WORDS - HaChunkOverheadWords()               */
/*                                                                           */
/*  words of memory for users of Ha.  This is the value of mem_total_words.  */
/*  We have to ensure that index is large enough for this to be positive.    */
/*  In fact, HA_MIN_CHUNK_INDEX is 4, which is fine.                         */
/*                                                                           */
/*****************************************************************************/

struct ha_chunk_rec {
  HA_CHUNK		next_chunk;		/* next chunk in chunk list  */
  size_t		mem_total_words;	/* total words in mem[]      */
  size_t		mem_avail_words;	/* avail words in mem[]      */
  int			index;			/* approx log_2(total_words) */
#if CHECK
  int			magic;
#endif
  HA_ALIGN_TYPE		mem[1];			/* extends */
};

typedef HA_ARRAY(HA_CHUNK)			ARRAY_CHUNK;


/*****************************************************************************/
/*                                                                           */
/*  HA_FREE_RESIZABLE_BLOCK_LIST - a list of free resizable blocks           */
/*                                                                           */
/*  arena                                                                    */
/*    The arena containing this object as well as its free blocks.           */
/*                                                                           */
/*  block_mem_words                                                          */
/*    The number of words available to users of Ha in each of the free       */
/*    blocks on this free block list.                                        */
/*                                                                           */
/*  first_free_resizable_block                                               */
/*    The first free resizable block, or NULL if none.                       */
/*                                                                           */
/*  next_free_resizable_block_list                                           */
/*    Another list of free resizable blocks, whose block_mem_words field     */
/*    is larger than this one's; or NULL if none.                            */
/*                                                                           */
/*****************************************************************************/

struct ha_free_resizable_block_list_rec {
  HA_ARENA			arena;
  size_t			block_mem_words;
  HA_FREE_RESIZABLE_BLOCK	first_free_resizable_block;
  HA_FREE_RESIZABLE_BLOCK_LIST	next_free_resizable_block_list;
#if CHECK
  int			magic;
#endif
};


/*****************************************************************************/
/*                                                                           */
/*  HA_RESIZABLE_BLOCK                                                       */
/*                                                                           */
/*  free_resizable_block_list                                                */
/*    The list of free blocks that this block will go to when freed.         */
/*                                                                           */
/*  block_magic                                                              */
/*    A magic number that proves that this block is (still) a block.  Its    */
/*    value should be FREE_BLOCK_CHECK_MAGIC for a free block, and           */
/*    BUSY_BLOCK_CHECK_MAGIC for a busy block.                               */
/*                                                                           */
/*****************************************************************************/

#if CHECK
#define INHERIT_HA_RESIZABLE_BLOCK					\
  HA_FREE_RESIZABLE_BLOCK_LIST	free_resizable_block_list;		\
  int				block_magic;
#else
#define INHERIT_HA_RESIZABLE_BLOCK					\
  HA_FREE_RESIZABLE_BLOCK_LIST	free_resizable_block_list;
#endif

typedef struct ha_resizable_block_rec {
  INHERIT_HA_RESIZABLE_BLOCK
} *HA_RESIZABLE_BLOCK;


/*****************************************************************************/
/*                                                                           */
/*  HA_FREE_RESIZABLE_BLOCK - a resizable block, currently free              */
/*                                                                           */
/*  INHERIT_HA_RESIZABLE_BLOCK                                               */
/*    Makes this type a subtype of HA_RESIZABLE_BLOCK.                       */
/*                                                                           */
/*  next_free_resizable_block                                                */
/*    A pointer to the next free block on this list, or NULL if none.        */
/*                                                                           */
/*  The block contains more memory than just these two fields; the amount    */
/*  is determined by free_resizable_block_list->block_mem_words.             */
/*                                                                           */
/*****************************************************************************/

struct ha_free_resizable_block_rec {
  INHERIT_HA_RESIZABLE_BLOCK
  HA_FREE_RESIZABLE_BLOCK		next_free_resizable_block;
};


/*****************************************************************************/
/*                                                                           */
/*  HA_BUSY_RESIZABLE_BLOCK - a resizable block, currently busy              */
/*                                                                           */
/*  INHERIT_HA_RESIZABLE_BLOCK                                               */
/*    Makes this type a subtype of HA_RESIZABLE_BLOCK.                       */
/*                                                                           */
/*  resizable[1]                                                             */
/*    Memory passed to the user of Ha.  The amount is given by               */
/*    free_resizable_block_list->block_mem_words.                            */
/*                                                                           */
/*****************************************************************************/

struct ha_busy_resizable_block_rec {
  INHERIT_HA_RESIZABLE_BLOCK
  HA_ALIGN_TYPE				resizable[1];		/* extends */
};


/*****************************************************************************/
/*                                                                           */
/*  HA_JUMP_ENV - what to do when memory runs out                            */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(HA_ARENA) ARRAY_ARENA;

typedef struct ha_jump_env_rec {
  jmp_buf		*env;			/* where to jump to       */
  ARRAY_ARENA		arenas;			/* free these before jump */
} *HA_JUMP_ENV;

typedef HA_ARRAY(HA_JUMP_ENV) ARRAY_JUMP_ENV;


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA - an arena (HA_ARENA is defined in howard_a.h)                  */
/*                                                                           */
/*  INHERIT_HA_RESIZABLE_BLOCK                                               */
/*    Makes this type a subtype of HA_RESIZABLE_BLOCK.                       */
/*                                                                           */
/*  arena_set                                                                */
/*    The enclosing arena set, always non-NULL.                              */
/*                                                                           */
/*  jump_env                                                                 */
/*    The most recent jump environment installed before this arena was       */
/*    created, or NULL if none.                                              */
/*                                                                           */
/*  first_busy_chunk                                                         */
/*    The first busy chunk of this arena.  By following this pointer to a    */
/*    chunk, and then the next_chunk fields until NULL is reached, you can   */
/*    visit every busy chunk of this arena.                                  */
/*                                                                           */
/*  Arenas do not hold free chunks; only arena sets do.                      */
/*                                                                           */
/*  By the end of initialization an arena always contains one free           */
/*  resizable block list, stored in one of its own chunks.  This needs       */
/*  to be installed explicitly by calling HaFreeResizableBlockListMake.      */
/*  The arena will also have at least one chunk, the one holding this        */
/*  free resizable block list, although no special action is required        */
/*  to bring this chunk into existence; the call to HaMake from within       */
/*  HaFreeResizableBlockListMake will do it naturally.                       */
/*                                                                           */
/*  The only objects stored in an arena set's private arena are the          */
/*  arena set object and the arena objects.  Other objects pointed to by     */
/*  these ones are not stored in the private arena and do not survive        */
/*  a call to HaDelete().                                                    */
/*                                                                           */
/*****************************************************************************/

struct ha_arena_rec {
  INHERIT_HA_RESIZABLE_BLOCK
  HA_ARENA_SET			arena_set;
  HA_JUMP_ENV			jump_env;
  HA_CHUNK			first_busy_chunk;
#if CHECK
  int				magic;
  bool				busy;
#endif
};


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA_SET - a set of arenas (HA_ARENA_SET is defined in howard_a.h)   */
/*                                                                           */
/*  private_arena                                                            */
/*    An arena, known only to this arena set, which holds the memory for     */
/*    the arena set object and for all its arena objects (but not for any    */
/*    other objects pointed to by those ones).  This is the first arena      */
/*    created and the last one deleted.                                      */
/*                                                                           */
/*  free_arenas                                                              */
/*    Arenas of this arena set that are currently free (unused).             */
/*                                                                           */
/*  busy_arenas_count                                                        */
/*    The number of arenas of this arena set that are currently busy (in     */
/*    use by Ha callers), not counting the private arena.  The busy arenas   */
/*    themselves are not accessible to this arena set, just their number.    */
/*                                                                           */
/*  free_chunk_lists                                                         */
/*    An array of lists of free chunks:  free_chunk_list[index] points to    */
/*    a list of free chunks whose index fields have value index.  These      */
/*    free chunks are linked together via their next_chunk fields as usual.  */
/*                                                                           */
/*  top_jmp_bufp                                                             */
/*    The top (last) element of jmp_bufps, duplicated here so that it can    */
/*    always be stored, even when there is not enough memory to add a new    */
/*    element to the end of jmp_bufps.                                       */
/*                                                                           */
/*  jmp_bufps                                                                */
/*    A stack of pointers to long_jmp environments.  The top element of      */
/*    the stack (the last element of the array, duplicated in top_jmp_bufp)  */
/*    is the place to jump to if we run out of memory.                       */
/*                                                                           */
/*  curr_words                                                               */
/*    The total number of words received from malloc by this arena set,      */
/*    including malloc and Ha overhead words.                                */
/*                                                                           */
/*  max_words                                                                */
/*    When non-zero, an upper limit on curr_words.  Any attempt to exceed    */
/*    this limit will be treated as though we are out of memory.             */
/*                                                                           */
/*****************************************************************************/

struct ha_arena_set_rec
{
  HA_ARENA		private_arena;		/* used only by as   */
  ARRAY_ARENA		free_arenas;		/* free arenas       */
  int			busy_arenas_count;	/* no, of busy arenas*/
  HA_CHUNK		free_chunk_lists[HA_MAX_INDEX];	/* indexed by index  */
  /* ARRAY_CHUNK	free_chunk_lists; */	/* indexed by index  */
  ARRAY_JUMP_ENV	free_jump_envs;		/* free jump envs    */
  ARRAY_JUMP_ENV	jump_envs;		/* longjmp envs      */
  size_t		curr_words;		/* curr words used   */
  size_t		max_words;		/* max words used    */
#if CHECK
  int			magic;
  ARRAY_ARENA		busy_arenas;		/* busy arenas       */
#endif
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Miscellaneous helper functions concerning bytes and words"    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  size_t HaWordsToBytes(size_t words)                                      */
/*                                                                           */
/*  Convert words (measured in words) to bytes.                              */
/*                                                                           */
/*****************************************************************************/

static size_t HaWordsToBytes(size_t words)
{
  return words * sizeof(HA_ALIGN_TYPE);
}


/*****************************************************************************/
/*                                                                           */
/*  size_t HaBytesToWords(size_t size)                                       */
/*                                                                           */
/*  Convert size (measured in bytes) to words, rounding up to the next       */
/*  whole number of words.                                                   */
/*                                                                           */
/*****************************************************************************/

static size_t HaBytesToWords(size_t size)
{
  return (size + (sizeof(HA_ALIGN_TYPE) - 1)) / sizeof(HA_ALIGN_TYPE);
}


/*****************************************************************************/
/*                                                                           */
/*  int HaChunkOverheadWords()                                               */
/*                                                                           */
/*  Return the number of words of overhead in a chunk.  This understands     */
/*  that there may be a gap between the last overhead field and mem[]; it    */
/*  includes any such gap.                                                   */
/*                                                                           */
/*****************************************************************************/

static size_t HaChunkOverheadWords()
{
  return HaBytesToWords(sizeof(struct ha_chunk_rec))
    - HaBytesToWords(sizeof(HA_ALIGN_TYPE));
}


/*****************************************************************************/
/*                                                                           */
/*  size_t HaBusyResizableBlockOverheadWords()                               */
/*                                                                           */
/*  Return the number of words of overhead in a busy resizable block.        */
/*                                                                           */
/*****************************************************************************/

static size_t HaBusyResizableBlockOverheadWords()
{
  return HaBytesToWords(sizeof(struct ha_busy_resizable_block_rec)) -
    HaBytesToWords(sizeof(HA_ALIGN_TYPE));
}


/*****************************************************************************/
/*                                                                           */
/*  size_t HaBusyResizableBlockOverheadBytes()                               */
/*                                                                           */
/*  Return the number of bytes of overhead in a busy resizable block.        */
/*                                                                           */
/*****************************************************************************/

static size_t HaBusyResizableBlockOverheadBytes()
{
  return sizeof(struct ha_busy_resizable_block_rec) - sizeof(HA_ALIGN_TYPE);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "HA_CHUNK"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HaChunkInitMagic(HA_CHUNK chunk)                                    */
/*                                                                           */
/*  Set the magic number field that proves that chunk is (still) a chunk.    */
/*                                                                           */
/*****************************************************************************/

static void HaChunkInitMagic(HA_CHUNK chunk)
{
#if CHECK
  chunk->magic = CHUNK_CHECK_MAGIC;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaChunkCheckMagic(HA_CHUNK chunk)                                   */
/*                                                                           */
/*  Check the magic number that proves that chunk is (still) a chunk.        */
/*                                                                           */
/*****************************************************************************/

static void HaChunkCheckMagic(HA_CHUNK chunk)
{
#if CHECK
  HnAssert(chunk->magic == CHUNK_CHECK_MAGIC, "HaChunkCheckMagic failed");
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaChunkInit(HA_CHUNK chunk, size_t words, int index,                */
/*    HA_CHUNK next_chunk)                                                   */
/*                                                                           */
/*  Initialize the fields of chunk (except mem[]) to the values given.       */
/*                                                                           */
/*****************************************************************************/

static void HaChunkInit(HA_CHUNK chunk, size_t words, int index,
  HA_CHUNK next_chunk)
{
  chunk->next_chunk = next_chunk;
  chunk->mem_total_words = chunk->mem_avail_words = words;
  chunk->index = index;
  HaChunkInitMagic(chunk);
}


/*****************************************************************************/
/*                                                                           */
/*  bool HaChunkHasMem(HA_CHUNK chunk, size_t words)                         */
/*                                                                           */
/*  Return true if chunk can supply words words of memory.                   */
/*                                                                           */
/*****************************************************************************/

static bool HaChunkHasMem(HA_CHUNK chunk, size_t words)
{
  HaChunkCheckMagic(chunk);
  return chunk->mem_avail_words >= words;
}


/*****************************************************************************/
/*                                                                           */
/*  void *HaChunkGetMem(HA_CHUNK chunk, size_t words)                        */
/*                                                                           */
/*  Return a pointer to size_t words of memory from chunk, assuming that     */
/*  chunk contains enough memory to satisfy this request.                    */
/*                                                                           */
/*****************************************************************************/

static void *HaChunkGetMem(HA_CHUNK chunk, size_t words)
{
  if( DEBUG3 )
    fprintf(stderr, "  HaChunkGetMem(chunk %lu, words %lu), avail %lu\n",
      (int64_t) chunk, (int64_t) words, (int64_t) chunk->mem_avail_words);
  HaChunkCheckMagic(chunk);
  return &chunk->mem[chunk->mem_avail_words -= words];
}


/*****************************************************************************/
/*                                                                           */
/*  void HaChunkListFree(HA_CHUNK chunk)                                     */
/*                                                                           */
/*  Free the list of chunks that begins with chunk.                          */
/*                                                                           */
/*  Implementation note.  We're assuming here that the contents of a chunk   */
/*  becomes undefined once it is freed, so we extract the next_chunk field   */
/*  of chunk before we free chunk.                                           */
/*                                                                           */
/*****************************************************************************/

static void HaChunkListFree(HA_CHUNK chunk)
{
  HA_CHUNK next_chunk;
  while( chunk != NULL )
  {
    HaChunkCheckMagic(chunk);
    next_chunk = chunk->next_chunk;
    if( DEBUG7 )
      fprintf(stderr, "  free %p\n", (void *) chunk);
    free(chunk);
    chunk = next_chunk;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *HaChunkShow(HA_CHUNK chunk)                                        */
/*                                                                           */
/*  Return a display of chunk.                                               */
/*                                                                           */
/*****************************************************************************/

/* *** good but currently unused
static char *HaChunkShow(HA_CHUNK chunk)
{
  static char buff[200];
  if( chunk == NULL )
    snprintf(buff, 200, "null chunk");
  else
    snprintf(buff, 200, "chunk %lu [mem %lu, avail %lu, total %lu]",
      (int64_t) chunk, (int64_t) &chunk->mem,
      (int64_t) &chunk->mem[chunk->mem_avail_words],
      (int64_t) &chunk->mem[chunk->mem_total_words]);
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "HA_FREE_RESIZABLE_BLOCK_LIST"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HaFreeResizableBlockListInitMagic(HA_FREE_RESIZABLE_BLOCK_LIST bl)  */
/*                                                                           */
/*  Set the magic number field that proves that bl is (still) a block list.  */
/*                                                                           */
/*****************************************************************************/

static void HaFreeResizableBlockListInitMagic(HA_FREE_RESIZABLE_BLOCK_LIST bl)
{
#if CHECK
  bl->magic = FREE_BLOCK_LIST_CHECK_MAGIC;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaFreeResizableBlockListCheckMagic(HA_FREE_RESIZABLE_BLOCK_LIST bl) */
/*                                                                           */
/*  Check the magic number that proves that bl is (still) a block list.      */
/*                                                                           */
/*****************************************************************************/
static void HaArenaCheckMagic(HA_ARENA a, bool busy);

static void HaFreeResizableBlockListCheckMagic(HA_FREE_RESIZABLE_BLOCK_LIST bl)
{
#if CHECK
  HnAssert(bl->magic == FREE_BLOCK_LIST_CHECK_MAGIC,
    "HaFreeResizableBlockListCheckMagic failed");
  HaArenaCheckMagic(bl->arena, true);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  HA_FREE_RESIZABLE_BLOCK_LIST HaFreeResizableBlockListMake(HA_ARENA a,    */
/*    size_t words)                                                          */
/*                                                                           */
/*  Make a block list for blocks whose mem[] fields have this many words.    */
/*                                                                           */
/*  Implementation note.  HaMake is concerned only with unresizable blocks.  */
/*  It has nothing to do with resizable blocks or resizable block lists, so  */
/*  there is no circularity in calling HaMake here.                          */
/*                                                                           */
/*****************************************************************************/

static HA_FREE_RESIZABLE_BLOCK_LIST HaFreeResizableBlockListMake(HA_ARENA a,
  size_t words)
{
  HA_FREE_RESIZABLE_BLOCK_LIST res;
  HaArenaCheckMagic(a, true);
  HaMake(res, a);
  res->arena = a;
  res->block_mem_words = words;
  res->first_free_resizable_block = NULL;
  res->next_free_resizable_block_list = NULL;
  HaFreeResizableBlockListInitMagic(res);
  if( DEBUG5 )
    fprintf(stderr, "  HaFreeResizableBlockListMake(a %lu, words %lu) = %lu\n",
      (int64_t) a, (int64_t) words, (int64_t) res);
  HaArenaCheckMagic(a, true);
  HaFreeResizableBlockListCheckMagic(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_BUSY_RESIZABLE_BLOCK HaFreeResizableBlockListGetBlock(                */
/*    HA_FREE_RESIZABLE_BLOCK_LIST bl, size_t words)                         */
/*                                                                           */
/*  Get a resizable block with words words or more from bl or from           */
/*  a later block list.  This will be a free block, but it is cast           */
/*  to a busy block for return.                                              */
/*                                                                           */
/*****************************************************************************/
static void *HaArenaAllocWords(HA_ARENA a, size_t words);
static HA_BUSY_RESIZABLE_BLOCK HaFreeResizableBlockToBusy(
  HA_FREE_RESIZABLE_BLOCK free_block);
static void HaBusyResizableBlockInitMagic(HA_BUSY_RESIZABLE_BLOCK block);
static void HaBusyResizableBlockCheckMagic(HA_BUSY_RESIZABLE_BLOCK block);

static HA_BUSY_RESIZABLE_BLOCK HaFreeResizableBlockListGetBlock(
  HA_FREE_RESIZABLE_BLOCK_LIST bl, size_t words)
{
  HA_BUSY_RESIZABLE_BLOCK res;

  /* find the right free resizable block list; create block lists as needed */
  HaFreeResizableBlockListCheckMagic(bl);
  if( DEBUG5 )
    fprintf(stderr, "[ HaFreeResizableBlockListGetBlock(bl %lu, words %lu)\n",
      (int64_t) bl, (int64_t) words);
  for( ; bl->block_mem_words < words;  bl = bl->next_free_resizable_block_list )
  {
    /* make sure there is a next block list, since this one won't do */
    if( bl->next_free_resizable_block_list == NULL )
    {
      bl->next_free_resizable_block_list =
	HaFreeResizableBlockListMake(bl->arena,
	  bl->block_mem_words == 0 ? HA_INIT_RESIZABLE_BLOCK_WORDS :
	  2 * bl->block_mem_words + 1);
    }
  }

  /* find a free block in bl, or allocate one */
  if( bl->first_free_resizable_block != NULL )
  {
    /* use an existing free block of bl */
    res = HaFreeResizableBlockToBusy(bl->first_free_resizable_block);
    bl->first_free_resizable_block =
      bl->first_free_resizable_block->next_free_resizable_block;
  }
  else
  {
    /* allocate a block containing bl->block_mem_words available words */
    res = (HA_BUSY_RESIZABLE_BLOCK)
      HaArenaAllocWords(bl->arena, bl->block_mem_words +
	HaBusyResizableBlockOverheadWords());
    res->free_resizable_block_list = bl;
    HaBusyResizableBlockInitMagic(res);
  }
  HaFreeResizableBlockListCheckMagic(bl);
  HaBusyResizableBlockCheckMagic(res);
  if( DEBUG5 )
    fprintf(stderr, "] HaFreeResizableBlockListGetBlock returning %lu\n",
      (int64_t) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void HaFreeResizableBlockListPutBlock(HA_FREE_RESIZABLE_BLOCK_LIST bl,   */
/*    HA_BUSY_RESIZABLE_BLOCK block)                                         */
/*                                                                           */
/*  Put block back into bl.  This is the inverse of GetBlock, except that    */
/*  GetBlock searches for a suitable block list, whereas PutBlock doesn't.   */
/*  The block is a busy block when received but this operation turns it      */
/*  into a free block.                                                       */
/*                                                                           */
/*****************************************************************************/
static HA_FREE_RESIZABLE_BLOCK HaBusyResizableBlockToFree(
  HA_BUSY_RESIZABLE_BLOCK busy_block);

static void HaFreeResizableBlockListPutBlock(HA_FREE_RESIZABLE_BLOCK_LIST bl,
  HA_BUSY_RESIZABLE_BLOCK block)
{
  HA_FREE_RESIZABLE_BLOCK free_block;
  HaBusyResizableBlockCheckMagic(block);
  HaFreeResizableBlockListCheckMagic(bl);
  free_block = HaBusyResizableBlockToFree(block);
  free_block->next_free_resizable_block = bl->first_free_resizable_block;
  bl->first_free_resizable_block = free_block;
}


/*****************************************************************************/
/*                                                                           */
/*  char *HaFreeResizableBlockListShow(HA_FREE_RESIZABLE_BLOCK_LIST bl)      */
/*                                                                           */
/*  Show bl.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** good but currently unused
static char *HaFreeResizableBlockListShow(HA_FREE_RESIZABLE_BLOCK_LIST bl)
{
  static char buff[200];
  snprintf(buff, 200, "free_bl %lu [arena %lu, words %lu, first %lu, next %lu]",
    (int64_t) bl, (int64_t) bl->arena, (int64_t) bl->block_mem_words,
    (int64_t) bl->first_free_resizable_block,
    (int64_t) bl->next_free_resizable_block_list);
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "HA_RESIZABLE_BLOCK"                                           */
/*                                                                           */
/*****************************************************************************/

/* nothing here, seemingly (abstract supertype) */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "HA_FREE_RESIZABLE_BLOCK"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HaFreeResizableBlockInitMagic(HA_FREE_RESIZABLE_BLOCK block)        */
/*                                                                           */
/*  Initialize the magic field of block.                                     */
/*                                                                           */
/*****************************************************************************/

static void HaFreeResizableBlockInitMagic(HA_FREE_RESIZABLE_BLOCK block)
{
#if CHECK
  block->block_magic = FREE_BLOCK_CHECK_MAGIC;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaFreeResizableBlockCheckMagic(HA_FREE_RESIZABLE_BLOCK block)       */
/*                                                                           */
/*  Check the magic field of block.                                          */
/*                                                                           */
/*****************************************************************************/

static void HaFreeResizableBlockCheckMagic(HA_FREE_RESIZABLE_BLOCK block)
{
#if CHECK
  HnAssert(block->block_magic == FREE_BLOCK_CHECK_MAGIC,
    "HaResizableBlockCheckMagic internal error");
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  HA_BUSY_RESIZABLE_BLOCK HaFreeResizableBlockToBusy(                      */
/*    HA_FREE_RESIZABLE_BLOCK free_block)                                    */
/*                                                                           */
/*  Return free_block converted into a busy block.                           */
/*                                                                           */
/*****************************************************************************/

static HA_BUSY_RESIZABLE_BLOCK HaFreeResizableBlockToBusy(
  HA_FREE_RESIZABLE_BLOCK free_block)
{
  HA_BUSY_RESIZABLE_BLOCK res;
  HaFreeResizableBlockCheckMagic(free_block);
  res = (HA_BUSY_RESIZABLE_BLOCK) free_block;
  HaBusyResizableBlockInitMagic(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *HaFreeResizableBlockShow(HA_FREE_RESIZABLE_BLOCK free_block)       */
/*                                                                           */
/*  Show this free block in static memory.                                   */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static char *HaFreeResizableBlockShow(HA_FREE_RESIZABLE_BLOCK free_block)
{
  static char buff[200];
  snprintf(buff, 200, "free_block %lu [bl %lu, next_free %lu]",
    (int64_t) free_block, (int64_t) free_block->free_resizable_block_list,
    (int64_t) free_block->next_free_resizable_block);
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "HA_BUSY_RESIZABLE_BLOCK"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HaBusyResizableBlockInitMagic(HA_BUSY_RESIZABLE_BLOCK block)        */
/*                                                                           */
/*  Initialize the magic field of block.                                     */
/*                                                                           */
/*****************************************************************************/

static void HaBusyResizableBlockInitMagic(HA_BUSY_RESIZABLE_BLOCK block)
{
#if CHECK
  block->block_magic = BUSY_BLOCK_CHECK_MAGIC;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaBusyResizableBlockCheckMagic(HA_BUSY_RESIZABLE_BLOCK block)       */
/*                                                                           */
/*  Check the magic field of block.                                          */
/*                                                                           */
/*****************************************************************************/

static void HaBusyResizableBlockCheckMagic(HA_BUSY_RESIZABLE_BLOCK block)
{
#if CHECK
  HnAssert(block->block_magic == BUSY_BLOCK_CHECK_MAGIC,
    "HaBusyResizableBlockCheckMagic internal error");
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  HA_FREE_RESIZABLE_BLOCK HaBusyResizableBlockToFree(                      */
/*    HA_BUSY_RESIZABLE_BLOCK busy_block)                                    */
/*                                                                           */
/*  Return busy_block converted into a free block.                           */
/*                                                                           */
/*****************************************************************************/

static HA_FREE_RESIZABLE_BLOCK HaBusyResizableBlockToFree(
  HA_BUSY_RESIZABLE_BLOCK busy_block)
{
  HA_FREE_RESIZABLE_BLOCK res;
  HaBusyResizableBlockCheckMagic(busy_block);
  res = (HA_FREE_RESIZABLE_BLOCK) busy_block;
  HaFreeResizableBlockInitMagic(res);
  return res;
}


/* *** currently unused
static char *HaBusyResizableBlockShow(HA_BUSY_RESIZABLE_BLOCK busy_block)
{
  static char buff[200];
  snprintf(buff, 200, "busy_block %lu [bl %lu]",
    (int64_t) busy_block, (int64_t) busy_block->free_resizable_block_list);
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "HA_ARENA"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HaArenaInitMagic(HA_ARENA a)                                         */
/*                                                                           */
/*  Set the magic number field that proves that a is (still) an arena.       */
/*                                                                           */
/*****************************************************************************/

static void HaArenaInitMagic(HA_ARENA a, bool busy)
{
#if CHECK
  a->magic = ARENA_CHECK_MAGIC;
  a->block_magic = BUSY_BLOCK_CHECK_MAGIC;
  a->busy = busy;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaCheckMagic(HA_ARENA a, bool busy)                            */
/*                                                                           */
/*  Check the magic number field that proves that a is (still) an arena.     */
/*  Also check whether the arena is busy or not.                             */
/*                                                                           */
/*****************************************************************************/

static void HaArenaCheckMagic(HA_ARENA a, bool busy)
{
#if CHECK
  HnAssert(a->magic == ARENA_CHECK_MAGIC, "HaArenaCheckMagic failed (a)");
  HnAssert(a->block_magic == BUSY_BLOCK_CHECK_MAGIC,
    "HaArenaCheckMagic failed (b)");
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaIncreaseToMinSize(int *index, size_t *two_to_index,          */
/*    size_t total_overhead, size_t words)                                   */
/*                                                                           */
/*  Increase *index until (2 ^ *index), returned in *two_to_index, is large  */
/*  enough to cover a demand for words words, allowing for total_overhead.   */
/*                                                                           */
/*****************************************************************************/

static void HaArenaIncreaseToMinSize(int *index, size_t *two_to_index,
  size_t total_overhead, size_t words)
{
  *two_to_index = (size_t) 1 << (size_t) *index;
  while( *two_to_index < words + total_overhead )
  {
    (*index)++;
    *two_to_index <<= (size_t) 1;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void *HaArenaAllocWords(HA_ARENA a, size_t words)                        */
/*                                                                           */
/*  Allocate an unresizable block containing words words.                    */
/*                                                                           */
/*****************************************************************************/
static HA_CHUNK HaArenaSetGetChunk(HA_ARENA_SET as, int index,
  size_t two_to_index, size_t total_overhead, HA_CHUNK next_chunk,
  bool fail_if_no_mem);

static void *HaArenaAllocWords(HA_ARENA a, size_t words)
{
  HA_CHUNK chunk, new_chunk;  size_t two_to_index, total_overhead;  int index;

  chunk = a->first_busy_chunk;
  if( chunk != NULL )
  {
    /* try the first (current) chunk, otherwise set index for a larger chunk */
    if( HaChunkHasMem(chunk, words) )
      return HaChunkGetMem(chunk, words);
    index = chunk->index + 1;
  }
  else
  {
    /* no current chunk, so set index for a new first chunk */
    index = HA_MIN_CHUNK_INDEX;
  }

  /* increase the chunk size by enough to cover words */
  total_overhead = MALLOC_OVERHEAD_WORDS + HaChunkOverheadWords();
  HaArenaIncreaseToMinSize(&index, &two_to_index, total_overhead, words);

  /* get a new chunk from the arena set and add it to a */
  new_chunk = HaArenaSetGetChunk(a->arena_set, index,
    two_to_index, total_overhead, chunk, false);
  if( new_chunk == NULL )
  {
    /* try again for any free chunk which is large enough for words */
    index = HA_MIN_CHUNK_INDEX;
    HaArenaIncreaseToMinSize(&index, &two_to_index, total_overhead, words);
    new_chunk = HaArenaSetGetChunk(a->arena_set, index,
      two_to_index, total_overhead, chunk, true);
    HnAssert(new_chunk != NULL, "HaArenaAllocWords internal error");
  }

  a->first_busy_chunk = new_chunk;
  return HaChunkGetMem(new_chunk, words);
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaInit(HA_ARENA a, HA_ARENA_SET as, HA_CHUNK chunk)            */
/*                                                                           */
/*  Initialize arena a using these attributes.  The call to                  */
/*  HaFreeResizableBlockListMake(a, 0) is safe provided chunk has space      */
/*  for the result object, or is well-defined and can supply a fresh chunk.  */
/*                                                                           */
/*****************************************************************************/

static void HaArenaInit(HA_ARENA a, HA_ARENA_SET as, HA_JUMP_ENV jump_env,
  HA_CHUNK chunk)
{
  a->arena_set = as;
  a->jump_env = jump_env;
  a->first_busy_chunk = chunk;
  HaArenaInitMagic(a, true);
  a->free_resizable_block_list = HaFreeResizableBlockListMake(a, 0);
  HaArenaCheckMagic(a, true);
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA HaArenaMake(HA_ARENA_SET as)                                    */
/*                                                                           */
/*  Make a new, empty arena which is not as's private arena.  The arena      */
/*  object is stored in memory taken from as's private arena.                */
/*                                                                           */
/*  The call to HaFreeResizableBlockListMake allocates memory from res       */
/*  (the result of this call) itself, which involves creating the first      */
/*  chunk of res.  So although this function sets res->first_busy_chunk      */
/*  to NULL, by the time it returns there will in fact be a first chunk.     */
/*                                                                           */
/*  The call to HaFreeResizableBlockListMake assumes that res is a fully     */
/*  functional arena, except not for resizable blocks.  So even though       */
/*  free_resizable_block_list must be the first field of res, so that res    */
/*  can masquerade as a resizable block, still it must be the last field     */
/*  initialized when creating res.                                           */
/*                                                                           */
/*  There are several points in this code where memory is or could be        */
/*  allocated.  If any of those allocations cause as to run out of           */
/*  memory, this function will fail in the usual out-of-memory way.          */
/*  But it is important here to not fail with a corrupted arena set          */
/*  (that can cause and has caused mysterious bugs), so this code            */
/*  ensures that all the memory is allocated before changing as.             */
/*                                                                           */
/*****************************************************************************/
static void HaArenaSetCheckMagic(HA_ARENA_SET as);
/* static char *HaArenaShow(HA_ARENA a); */

HA_ARENA HaArenaMake(HA_ARENA_SET as)
{
  HA_ARENA res;  HA_JUMP_ENV jump_env;  int pos;

  /*************************************************************************/
  /*                                                                       */
  /*  code that does not commit as to the new arena                        */
  /*                                                                       */
  /*************************************************************************/

  /* grab an arena object from as->free_arenas, or make a new one */
  HaArenaSetCheckMagic(as);
  if( HaArrayCount(as->free_arenas) > 0 )
  {
    res = HaArrayLastAndDelete(as->free_arenas);
#if CHECK
    HnAssert(res != NULL && !res->busy, "HaArenaMake internal error 1");
#endif
  }
  else
    HaMake(res, as->private_arena);

  /* sort out the jump environment and add a space for res to it */
  if( HaArrayCount(as->jump_envs) > 0 )
  {
    jump_env = HaArrayLast(as->jump_envs);
    HaArrayAddLast(jump_env->arenas, NULL);
    /* HaArrayAddLast(jump_env->arenas, res); save until after commit */
  }
  else
    jump_env = NULL;

  /* initialize the fields of res */
  HaArenaInit(res, as, jump_env, NULL);

  /* add a space for res to busy_arenas */
#if CHECK
  HnAssert(!HaArrayContains(as->busy_arenas, res, &pos),
    "HaArenaMake internal error 2");
  HaArrayAddLast(as->busy_arenas, NULL);
  /* HaArrayAddLast(as->busy_arenas, res); save until after commit */
#endif

  /*************************************************************************/
  /*                                                                       */
  /*  commit point; no memory allocation below here                        */
  /*                                                                       */
  /*************************************************************************/

  /* add res to jump_env */
  if( jump_env != NULL )
    HaArrayPut(jump_env->arenas, HaArrayCount(jump_env->arenas) - 1, res);

  /* increment as->busy_arenas_count and return res */
  as->busy_arenas_count++;
#if CHECK
  HaArrayPut(as->busy_arenas, HaArrayCount(as->busy_arenas) - 1, res);
#endif
  HaArenaCheckMagic(res, true);
  HaArenaSetCheckMagic(as);
  if( DEBUG6 )
    fprintf(stderr, "  HaArenaMake returning %p in arena set %p, jump env %p\n",
      (void *) res, (void *) as, (void *) jump_env);
    /* ***
    fprintf(stderr, "  HaArenaMake returning %s (index %d)\n", HaArenaShow(res),
      res->first_busy_chunk != NULL ? res->first_busy_chunk->index : -1);
    *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaDelete(HA_ARENA a)                                           */
/*                                                                           */
/*  Delete arena a, which is not any arena set's private arena.  This        */
/*  returns a's chunks to its arena set, and adds a to its arena set's       */
/*  list of free arenas.                                                     */
/*                                                                           */
/*  It is not safe to delete an arena set's private arena by calling this    */
/*  function, so we don't.                                                   */
/*                                                                           */
/*****************************************************************************/
static void HaArenaSetAddChunkListToFreeChunks(HA_ARENA_SET as, HA_CHUNK chunk);

void HaArenaDelete(HA_ARENA a)
{
  HA_ARENA_SET as;  int pos;  HA_JUMP_ENV jump_env;

  /*************************************************************************/
  /*                                                                       */
  /*  code that does not commit as to the deletion                         */
  /*                                                                       */
  /*************************************************************************/

  /* add a to as's list of free arenas (do it first in case it fails) */
  HaArenaCheckMagic(a, true);
  if( DEBUG6 )
    fprintf(stderr, "  HaArenaDelete(%p) in arena set %p, jump env %p\n",
      (void *) a, (void *) a->arena_set, (void *) a->jump_env);
    /* fprintf(stderr, "  HaArenaDelete(%s)\n", HaArenaShow(a)); */
  as = a->arena_set;
  HaArenaSetCheckMagic(as);
  HaArrayAddLast(as->free_arenas, NULL);  /* put a later after committing */

  /*************************************************************************/
  /*                                                                       */
  /*  commit point; no memory allocation below here                        */
  /*                                                                       */
  /*************************************************************************/

#if CHECK
  a->busy = false;
#endif

  /* remove a from its jump environment, if any */
  jump_env = a->jump_env;
  if( jump_env != NULL )
  {
    if( !HaArrayContains(jump_env->arenas, a, &pos) )
      HnAbort("HaArenaDelete internal error 1");
    HaArrayDeleteAndPlug(jump_env->arenas, pos);
  }

  /* return the chunks of a to as and decrement as's busy arenas count */
  HaArenaSetAddChunkListToFreeChunks(as, a->first_busy_chunk);
  as->busy_arenas_count--;
#if CHECK
  if( !HaArrayContains(as->busy_arenas, a, &pos) )
    HnAbort("HaArenaDelete internal error 2");
  HaArrayDeleteAndPlug(as->busy_arenas, pos);
  HnAssert(HaArrayCount(as->busy_arenas) == as->busy_arenas_count,
    "HaArenaDelete internal error 3");
  a->busy = false;
#endif
  HaArenaSetCheckMagic(as);

  /* add a to free arenas list in previously allocated spot */
  HaArrayPut(as->free_arenas, HaArrayCount(as->free_arenas) - 1, a);
}


/*****************************************************************************/
/*                                                                           */
/*  void *HaAlloc(HA_ARENA a, size_t size)                                   */
/*                                                                           */
/*  Return a pointer to an unresizable block of at least size bytes of       */
/*  otherwise unallocated memory from arena a.                               */
/*                                                                           */
/*****************************************************************************/

void *HaAlloc(HA_ARENA a, size_t size)
{
  return HaArenaAllocWords(a, HaBytesToWords(size));
}


/*****************************************************************************/
/*                                                                           */
/*  void HaResizableCheckMagic(void *resizable)                              */
/*                                                                           */
/*  Check that resizable appears to be a resizable block.                    */
/*                                                                           */
/*****************************************************************************/

static void HaResizableCheckMagic(void *resizable)
{
/* *** currently causing an infinite recurse
#if CHECK
  HaArenaCheckMagic(HaResizableArena(resizable));
#endif
*** */
}


/*****************************************************************************/
/*                                                                           */
/*  HA_BUSY_RESIZABLE_BLOCK HaResizableBusyResizableBlock(void *resizable)   */
/*                                                                           */
/*  Return the busy resizable block containing resizable.                    */
/*                                                                           */
/*****************************************************************************/

static HA_BUSY_RESIZABLE_BLOCK HaResizableBusyResizableBlock(void *resizable)
{
  HA_BUSY_RESIZABLE_BLOCK res;
  HaResizableCheckMagic(resizable);
  res = (HA_BUSY_RESIZABLE_BLOCK)
    ((char *) resizable - HaBusyResizableBlockOverheadBytes());
  HaBusyResizableBlockCheckMagic(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_FREE_RESIZABLE_BLOCK_LIST HaResizableFreeResizableBlockList(          */
/*    void *resizable)                                                       */
/*                                                                           */
/*  Return the free resizable block list that resizable comes from.          */
/*                                                                           */
/*****************************************************************************/

static HA_FREE_RESIZABLE_BLOCK_LIST HaResizableFreeResizableBlockList(
  void *resizable)
{
  HA_BUSY_RESIZABLE_BLOCK block;
  HaResizableCheckMagic(resizable);
  block = HaResizableBusyResizableBlock(resizable);
  HnAssert(block->free_resizable_block_list != NULL,
    "HaResizableFreeResizableBlockList internal error");
  HaFreeResizableBlockListCheckMagic(block->free_resizable_block_list);
  return block->free_resizable_block_list;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA HaResizableArena(void *resizable)                               */
/*                                                                           */
/*  Return *resizable's arena.                                               */
/*                                                                           */
/*****************************************************************************/

HA_ARENA HaResizableArena(void *resizable)
{
  HA_FREE_RESIZABLE_BLOCK_LIST bl;
  /* can't check magic here, that would be infinite recursion */
  bl = HaResizableFreeResizableBlockList(resizable);
  return bl->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  size_t HaResizableSize(void *resizable)                                  */
/*                                                                           */
/*  Return the size of *resizable, in bytes.                                 */
/*                                                                           */
/*****************************************************************************/

size_t HaResizableSize(void *resizable)
{
  HA_FREE_RESIZABLE_BLOCK_LIST bl;
  HaResizableCheckMagic(resizable);
  bl = HaResizableFreeResizableBlockList(resizable);
  return HaWordsToBytes(bl->block_mem_words);
}


/*****************************************************************************/
/*                                                                           */
/*  void *HaResizableAlloc(HA_ARENA a)                                       */
/*                                                                           */
/*  Return a resizable block of size 0 from a.                               */
/*                                                                           */
/*  Implementation note.  We use arena a itself as this block.  All that     */
/*  a block of size 0 needs is a u field, and a has that.                    */
/*                                                                           */
/*****************************************************************************/

void *HaResizableAlloc(HA_ARENA a)
{
  HA_BUSY_RESIZABLE_BLOCK arena_as_block;  void *res;

  /* make sure that a has a non-NULL free resizable block list */
  /* *** there from the start now
  if( a->free_resizable_block_list == NULL )
    a->free_resizable_block_list = HaFreeResizableBlockListMake(a, 0);
  *** */

  /* return a itself as the block */
  HaArenaCheckMagic(a, true);
  HnAssert(a != NULL, "HaResizableAlloc internal error");
  arena_as_block = (HA_BUSY_RESIZABLE_BLOCK) a;
  if( DEBUG1 )
    fprintf(stderr, "HaResizableAlloc(%p) returning %p\n",
      (void *) a, (void *) &arena_as_block->resizable[0]);
  res = &arena_as_block->resizable[0];
  if( DEBUG4 )
    fprintf(stderr, "  HaResizableAlloc returning %lu (arena is %lu)\n",
      (int64_t) res, (int64_t) HaResizableArena(res));
  HaResizableCheckMagic(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void *HaResizableReAlloc(void *resizable, size_t size)                   */
/*                                                                           */
/*  Reallocate *resizable to a new block of at least size bytes, assuming    */
/*  that *resizable was allocated by HaResizableAlloc or HaResizableReAlloc. */
/*                                                                           */
/*****************************************************************************/

void *HaResizableReAlloc(void *resizable, size_t size)
{
  HA_FREE_RESIZABLE_BLOCK_LIST old_bl;  HA_BUSY_RESIZABLE_BLOCK old_block;
  size_t words;  HA_BUSY_RESIZABLE_BLOCK new_block;  void *res;

  /* find resizable's block and block list */
  if( DEBUG4 )
    fprintf(stderr, "[ HaResizableReAlloc(%lu, %lu)\n",
      (int64_t) resizable, (int64_t) size);
  HaResizableCheckMagic(resizable);
  old_block = HaResizableBusyResizableBlock(resizable);  /* arena if empty */
  old_bl = old_block->free_resizable_block_list;  /* correct even if arena */

  /* return resizable if it's enough */
  words = HaBytesToWords(size);
  if( old_bl->block_mem_words >= words )
  {
    if( DEBUG4 )
      fprintf(stderr, "] HaResizableReAlloc returning orig. %lu\n",
	(int64_t) resizable);
    return resizable;
  }

  /* get a sufficiently large new block and copy resizable into it */
  new_block = HaFreeResizableBlockListGetBlock(old_bl, words);
  memcpy(&new_block->resizable[0], resizable,
    HaWordsToBytes(old_bl->block_mem_words));

  /* add old_block to its block list's free list, unless empty */
  if( old_bl->block_mem_words > 0 )
    HaFreeResizableBlockListPutBlock(old_bl, old_block);

  /* return new_block's mem */
  res = &new_block->resizable[0];
  if( DEBUG4 )
    fprintf(stderr, "] HaResizableReAlloc returning %lu\n", (int64_t) res);
  HaResizableCheckMagic(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void HaResizableFree(void *resizable)                                    */
/*                                                                           */
/*  Free resizable, assuming it was allocated by HaResizableAlloc or         */
/*  HaResizableReAlloc.                                                      */
/*                                                                           */
/*****************************************************************************/

void HaResizableFree(void *resizable)
{
  HA_FREE_RESIZABLE_BLOCK_LIST old_bl;  HA_BUSY_RESIZABLE_BLOCK old_block;

  /* find resizable's block and block list */
  HaResizableCheckMagic(resizable);
  old_block = HaResizableBusyResizableBlock(resizable);  /* arena if empty */
  old_bl = old_block->free_resizable_block_list;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ HaResizableFree(%p)\n", resizable);
    HaArenaDebug(old_bl->arena, 1, 2, stderr);
  }

  /* add old_block to its block list's free list, if it has non-zero size */
  if( old_bl->block_mem_words > 0 )
    HaFreeResizableBlockListPutBlock(old_bl, old_block);
  if( DEBUG1 )
  {
    fprintf(stderr, "arena after HaResizableFree:\n");
    HaArenaDebug(old_bl->arena, 1, 2, stderr);
    fprintf(stderr, "] HaResizableFree returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaCheck(HA_ARENA a)                                            */
/*                                                                           */
/*  Carry out a check of arena a.                                            */
/*                                                                           */
/*****************************************************************************/

void HaArenaCheck(HA_ARENA a)
{
#if CHECK
  HA_CHUNK chunk;
  HaArenaCheckMagic(a, true);
  HaArenaSetCheckMagic(a->arena_set);
  for( chunk = a->first_busy_chunk;  chunk != NULL;  chunk = chunk->next_chunk )
    HaChunkCheckMagic(chunk);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaDebug(HA_ARENA a, int verbosity, int indent, FILE *fp)       */
/*                                                                           */
/*  Debug print of arena a.                                                  */
/*                                                                           */
/*****************************************************************************/

void HaArenaDebug(HA_ARENA a, int verbosity, int indent, FILE *fp)
{
  HA_CHUNK chunk;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ HaArena(%p)\n", indent, "", (void *) a);
    fprintf(fp, "%*s  busy:", indent, "");
    for( chunk = a->first_busy_chunk; chunk != NULL; chunk = chunk->next_chunk )
      fprintf(fp, " %ld", chunk->mem_total_words);
    fprintf(fp, "\n");
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *HaArenaShow(HA_ARENA a)                                            */
/*                                                                           */
/*  Show a.                                                                  */
/*                                                                           */
/*****************************************************************************/

/* *** good but currently unused
static char *HaArenaShow(HA_ARENA a)
{
  static char buff[600];
  snprintf(buff, 600, "arena %p [free_bl %s, arena_set %p, first %s]",
    (void *) a, HaFreeResizableBlockListShow(a->free_resizable_block_list),
    (void *) a->arena_set, HaChunkShow(a->first_busy_chunk));
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "HA_ARENA_SET"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetInitMagic(HA_ARENA_SET as)                                 */
/*                                                                           */
/*  Set the magic number field that proves that as is (still) an arena set.  */
/*                                                                           */
/*****************************************************************************/

static void HaArenaSetInitMagic(HA_ARENA_SET as)
{
#if CHECK
  as->magic = ARENA_SET_CHECK_MAGIC;
  HaArrayInit(as->busy_arenas, as->private_arena);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetCheckMagic(HA_ARENA_SET as)                               */
/*                                                                           */
/*  Check the magic number that proves that as is (still) an arena set.      */
/*                                                                           */
/*****************************************************************************/

static void HaArenaSetCheckMagic(HA_ARENA_SET as)
{
#if CHECK
  HnAssert(as->magic == ARENA_SET_CHECK_MAGIC, "HaArenaSetCheckMagic failed");
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetOutOfMemory(HA_ARENA_SET as, int index)                   */
/*                                                                           */
/*  Arena set as is out of memory, so do a longjmp or abort.  Parameter      */
/*  index is the index of the size of chunk that we failed to get, and       */
/*  is used for debugging only.                                              */
/*                                                                           */
/*  This function leaves it to the caller to remove the top jump             */
/*  environment from the stack (by calling HaArenaSetJmpEnvEnd).             */
/*                                                                           */
/*  The last element of jump_env->arenas could be NULL; this may occur       */
/*  when memory runs out at an awkward moment within HaArenaMake.            */
/*                                                                           */
/*****************************************************************************/

static void HaArenaSetOutOfMemory(HA_ARENA_SET as, int index)
{
  HA_JUMP_ENV jump_env;  HA_ARENA a;
  HaArenaSetCheckMagic(as);
  /* HnAbort("HaArenaSetOutOfMemory temporary internal error"); */
  if( DEBUG9 )
  {
    fprintf(stderr, "  HaArenaSetOutOfMemory(%p, %d):\n", (void *) as, index);
    HaArenaSetDebug(as, 2, 4, stderr);
  }
  if( HaArrayCount(as->jump_envs) > 0 )
  {
    /* delete arenas created since the top jump env was installed */
    jump_env = HaArrayLast(as->jump_envs);
    while( HaArrayCount(jump_env->arenas) > 0 )
    {
      a = HaArrayLast(jump_env->arenas);
      if( a == NULL )
	HaArrayDeleteLast(jump_env->arenas);
      else
      {
	HnAssert(a->jump_env == jump_env,
	  "HaArenaSetOutOfMemory internal error 2");
	HaArenaDelete(a);
      }
    }

    /* take the long jump */
    longjmp(*jump_env->env, 1);
  }
  else
  {
    /* abort */
    HnAbort("HaArenaSetGetChunk: out of memory");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  HA_CHUNK HaArenaSetDoGetChunk(HA_ARENA_SET as, int index,                */
/*    size_t two_to_index, size_t total_overhead, HA_CHUNK next_chunk)       */
/*                                                                           */
/*  Get a chunk, given that there isn't a suitable one in the free lists.    */
/*                                                                           */
/*****************************************************************************/

/* *** inlined now
static HA_CHUNK HaArenaSetDoGetChunk(HA_ARENA_SET as, int index,
  size_t two_to_index, size_t total_overhead, HA_CHUNK next_chunk)
{
  HA_CHUNK res;
  HaArenaSetCheckMagic(as);
  if( as->max_words > 0 && as->curr_words + two_to_index > as->max_words )
  {
    ** allocation would exceed memory limit, so jump or abort instead **
    HaArenaSetOutOfMemory(as);
    res = NULL;  ** keep compiler happy **
  }
  else
  {
    ** get res from calloc() **
    res = (HA_CHUNK) calloc(two_to_index - MALLOC_OVERHEAD_WORDS,
      sizeof(HA_ALIGN_TYPE));
    if( res == NULL )
      HaArenaSetOutOfMemory(as);
    if( DEBUG7 )
      fprintf(stderr, "  calloc (a) returning %p (%lu words)\n",
	(void *) res, two_to_index - MALLOC_OVERHEAD_WORDS);
    as->curr_words += two_to_index;
    HaChunkInit(res, two_to_index - total_overhead, index, next_chunk);
  }
  HaArenaSetCheckMagic(as);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  HA_CHUNK HaArenaSetGetChunk(HA_ARENA_SET as, int index,                  */
/*    size_t two_to_index, size_t total_overhead, HA_CHUNK next_chunk,       */
/*    bool fail_if_no_mem)                                                   */
/*                                                                           */
/*  Get a chunk with the given index from as.  Here two_to_index is          */
/*  2^index, total_overhead is the chunk overhead, including both the        */
/*  malloc overhead and the Ha overhead, and next_chunk is a value to        */
/*  set the result's next_chunk field to.                                    */
/*                                                                           */
/*  Implementation note.  It's tempting to call HaArrayFill to enlarge       */
/*  as->free_chunk_lists when index is off the end, but that leads to        */
/*  infinite recursion.  So we don't enlarge until the arrival of a          */
/*  free chunk obliges us to.                                                */
/*                                                                           */
/*****************************************************************************/

static HA_CHUNK HaArenaSetGetChunk(HA_ARENA_SET as, int index,
  size_t two_to_index, size_t total_overhead, HA_CHUNK next_chunk,
  bool fail_if_no_mem)
{
  HA_CHUNK res;  int index2;
  HaArenaSetCheckMagic(as);
  if( DEBUG8 )
  {
    fprintf(stderr, "[ HaArenaSetGetChunk(as, %d, %ld, %ld, %p)\n",
      index, two_to_index, total_overhead, (void *) next_chunk);
    HnAssert(index < 24, "HaArenaSetGetChunk aborting on large index");
  }

  /* get res from the smallest available free list, if any */
  /* ***
  for( index2 = index; index2 < HaArrayCount(as->free_chunk_lists);  index2++ )
  *** */
  for( index2 = index; index2 < HA_MAX_INDEX;  index2++ )
  {
    /* res = HaArray(as->free_chunk_lists, index2); */
    res = as->free_chunk_lists[index2];
    if( res != NULL )
    {
      as->free_chunk_lists[index2] = res->next_chunk;
      HnAssert(res->mem_total_words >= two_to_index - total_overhead,
	"HaArenaSetGetChunk internal error");
      HaChunkInit(res, res->mem_total_words, index2, next_chunk);
      memset(&res->mem, 0, res->mem_total_words * sizeof(HA_ALIGN_TYPE));
      if( DEBUG8 )
	fprintf(stderr, "] HaArenaSetGetChunk returning arena set chunk %p\n",
	  (void *) res);
      HaArenaSetCheckMagic(as);
      HaChunkCheckMagic(res);
      return res;
    }
  }

  /* jump, abort, or return NULL if internal memory limit reached */
  if( as->max_words > 0 && as->curr_words + two_to_index > as->max_words )
  {
    if( fail_if_no_mem )
      HaArenaSetOutOfMemory(as, index);
    else
    {
      if( DEBUG8 )
	fprintf(stderr, "] HaArenaSetGetChunk returning NULL (mem limit)\n");
      return NULL;
    }
  }

  /* get res from malloc */
  res = (HA_CHUNK) malloc(
    (two_to_index - MALLOC_OVERHEAD_WORDS) * sizeof(HA_ALIGN_TYPE));

  /* jump, abort, or return NULL if malloc did not return any memory */
  if( res == NULL )
  {
    if( fail_if_no_mem )
      HaArenaSetOutOfMemory(as, index);
    else
    {
      if( DEBUG8 )
	fprintf(stderr, "] HaArenaSetGetChunk returning NULL (malloc)\n");
      return NULL;
    }
  }

  /* success; initialize res and return it */
  if( DEBUG7 )
    fprintf(stderr, "  malloc (a) returning %p (%lu words)\n",
      (void *) res, two_to_index - MALLOC_OVERHEAD_WORDS);
  as->curr_words += two_to_index;
  HaChunkInit(res, two_to_index - total_overhead, index, next_chunk);
  if( DEBUG8 )
    fprintf(stderr, "] HaArenaSetGetChunk returning malloc chunk %p\n",
      (void *) res);
  HaArenaSetCheckMagic(as);
  HaChunkCheckMagic(res);
  return res;
}


/* ***
static HA_CHUNK HaArenaSetGetChunk(HA_ARENA_SET as, int index,
  size_t two_to_index, size_t total_overhead, HA_CHUNK next_chunk)
{
  HA_CHUNK res;
  HaArrayFill(as->free_chunk_lists, index + 1, NULL);
  res = HaArray(as->free_chunk_lists, index);
  if( res != NULL )
  {
    ** get res from the free list **
    HaArrayPut(as->free_chunk_lists, index, res->next_chunk);
    HnAssert(res->mem_total_words == two_to_index - total_overhead,
      "HaArenaSetGetChunk internal error");
    HaChunkInit(res, res->mem_total_words, index, next_chunk);
  }
  else if( as->max_words > 0 && as->curr_words + two_to_index > as->max_words )
  {
    ** allocation would exceed memory limit, so jump or abort instead **
    HaArenaSetOutOfMemory(as);
  }
  else
  {
    ** get res from calloc() **
    res = (HA_CHUNK) calloc(two_to_index - MALLOC_OVERHEAD_WORDS,
      sizeof(HA_ALIGN_TYPE));
    if( res == NULL )
      HaArenaSetOutOfMemory(as);
    as->curr_words += two_to_index;
    HaChunkInit(res, two_to_index - total_overhead, index, next_chunk);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetPutChunk(HA_ARENA_SET as, HA_CHUNK chunk)                 */
/*                                                                           */
/*  Add chunk to a free list in as.                                          */
/*                                                                           */
/*****************************************************************************/

static void HaArenaSetPutChunk(HA_ARENA_SET as, HA_CHUNK chunk)
{
  HaArenaSetCheckMagic(as);
  HaChunkCheckMagic(chunk);
  if( DEBUG8 )
    fprintf(stderr, "[ HaArenaSetPutChunk(as, %p) ]\n", (void *) chunk);
  /* HaArrayFill(as->free_chunk_lists, chunk->index + 1, NULL); */
  HnAssert(chunk->index < HA_MAX_INDEX, "HaArenaSetPutChunk: chunk too large");
  chunk->next_chunk = as->free_chunk_lists[chunk->index];
  as->free_chunk_lists[chunk->index] = chunk;
  HaArenaSetCheckMagic(as);
  HaChunkCheckMagic(chunk);
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetAddChunkListToFreeChunks(HA_ARENA_SET as, HA_CHUNK chunk) */
/*                                                                           */
/*  Add the chunks from the list beginning with chunk to as's free chunks.   */
/*                                                                           */
/*****************************************************************************/

static void HaArenaSetAddChunkListToFreeChunks(HA_ARENA_SET as, HA_CHUNK chunk)
{
  HA_CHUNK next_chunk;
  HaArenaSetCheckMagic(as);
  while( chunk != NULL )
  {
    HaChunkCheckMagic(chunk);
    next_chunk = chunk->next_chunk;
    HaArenaSetPutChunk(as, chunk);
    chunk = next_chunk;
  }
  HaArenaSetCheckMagic(as);
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA_SET HaArenaSetMake(void)                                        */
/*                                                                           */
/*****************************************************************************/

HA_ARENA_SET HaArenaSetMake(void)
{
  HA_ARENA a;  HA_ARENA_SET as;  HA_CHUNK chunk;
  size_t min_words, words, as_words, a_words, bl_words;  int index;

  /* ***
  if( DEBUG2 )
    fprintf(stderr, "[ HaArenaSetMake(void)\n");
  *** */

  /* align type must hold at least a pointer */
  HnAssert(sizeof(HA_ALIGN_TYPE) >= sizeof(void *),
    "HaArenaSetMake: HA_ALIGN_TYPE is too small (must hold pointer at least)");

  /* get a chunk large enough to hold one arena set, arena, and block list */
  as_words = HaBytesToWords(sizeof(*as));
  a_words = HaBytesToWords(sizeof(*a));
  bl_words = HaBytesToWords(sizeof(struct ha_free_resizable_block_list_rec));
  min_words = as_words + a_words + bl_words + HaChunkOverheadWords();
  for( words = 1, index = 0;  words < min_words;  words *= 2, index++ );
  chunk = (HA_CHUNK) calloc(words - MALLOC_OVERHEAD_WORDS,
    sizeof(HA_ALIGN_TYPE));
  if( DEBUG3 )
    fprintf(stderr, "  chunk = calloc(%lu, %lu) = [%ld .. %ld)\n",
      words - MALLOC_OVERHEAD_WORDS, sizeof(HA_ALIGN_TYPE), (int64_t) chunk,
      (int64_t) chunk +
      (int64_t) (words - MALLOC_OVERHEAD_WORDS) * (int64_t) sizeof(HA_ALIGN_TYPE));
  if( chunk == NULL )
    HnAbort("HaArenaSetMake aborting (out of memory)");
  if( DEBUG7 )
    fprintf(stderr, "  calloc (b) returning %p (%lu words)\n",
      (void *) chunk, words - MALLOC_OVERHEAD_WORDS);
  HaChunkInit(chunk, words - MALLOC_OVERHEAD_WORDS - HaChunkOverheadWords(),
    index, NULL);


  /* get memory from chunk for the arena set and its private arena */
  as = (HA_ARENA_SET) HaChunkGetMem(chunk, as_words);
  a = (HA_ARENA) HaChunkGetMem(chunk, a_words);
  if( DEBUG3 )
  {
    fprintf(stderr, "  as = [%ld .. %ld]\n",
	(int64_t) as, (int64_t) as + (int64_t) sizeof(*as));
    fprintf(stderr, "  a  = [%ld .. %ld]\n",
	(int64_t) a , (int64_t) a  + (int64_t) sizeof(*a));
  }

  /* initialize the private arena */
  HaArenaInit(a, as, NULL, chunk);

  /* initialize the arena set */
  as->private_arena = a;
  HaArrayInit(as->free_arenas, a);
  as->busy_arenas_count = 0;  /* we don't count the private arena */
  for( index = 0;  index < HA_MAX_INDEX;  index++ )
    as->free_chunk_lists[index] = NULL;
  /* HaArrayInit(as->free_chunk_lists, a); */
  HaArrayInit(as->free_jump_envs, a);
  HaArrayInit(as->jump_envs, a);
  as->curr_words = words;
  as->max_words = 0;  /* means no limit */
  HaArenaSetInitMagic(as);

  /* all good, ready to go */
  HaArenaCheckMagic(a, true);
  HaArenaSetCheckMagic(as);
  if( DEBUG2 )
    fprintf(stderr, "  HaArenaSetMake returning %p\n", (void *) as);
  return as;
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetMerge(HA_ARENA_SET dest_as, HA_ARENA_SET src_as)          */
/*                                                                           */
/*  Merge the arenas of src_as into dest_as.                                 */
/*                                                                           */
/*****************************************************************************/

void HaArenaSetMerge(HA_ARENA_SET dest_as, HA_ARENA_SET src_as)
{
  int index;  HA_CHUNK chunk;

  /* make sure src_as has no remaining busy arenas */
  if( DEBUG2 )
    fprintf(stderr, "  HaArenaSetMerge(%p, %p)\n", (void *) dest_as,
      (void *) src_as);
  HaArenaSetCheckMagic(src_as);
  HaArenaSetCheckMagic(dest_as);
  HnAssert(src_as->busy_arenas_count == 0,
    "HaArenaSetMerge: src_as %p has %d busy arenas", (void *) src_as,
    src_as->busy_arenas_count);

  /* move across all src_as's free chunks */
  /* ***
  for( index = 0;  index < HaArrayCount(src_as->free_chunk_lists);  index++ )
  *** */
  for( index = 0;  index < HA_MAX_INDEX;  index++ )
  {
    chunk = src_as->free_chunk_lists[index];
    HaArenaSetAddChunkListToFreeChunks(dest_as, chunk);
  }

  /* move across the busy chunks from src_as's private arena */
  chunk = src_as->private_arena->first_busy_chunk;
  HaArenaSetAddChunkListToFreeChunks(dest_as, chunk);
  HaArenaSetCheckMagic(dest_as);
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetDelete(HA_ARENA_SET as)                                   */
/*                                                                           */
/*  Delete as and its arenas, returning all memory to the operating system.  */
/*                                                                           */
/*  Implementation note.  We free the chunks of the private arena last, and  */
/*  separately, because that arena contains as and as->free_chunk_lists.     */
/*                                                                           */
/*****************************************************************************/

void HaArenaSetDelete(HA_ARENA_SET as)
{
  int index;  HA_CHUNK chunk;

  /* make sure there are no remaining busy arenas */
  if( DEBUG2 )
    fprintf(stderr, "  HaArenaSetDelete(%p)\n", (void *) as);
  HaArenaSetCheckMagic(as);
  HnAssert(as->busy_arenas_count == 0,
    "HaArenaSetDelete: as has %d busy arenas", as->busy_arenas_count);

  /* free all chunks except chunks from the private arena */
  /* for( index = 0;  index < HaArrayCount(as->free_chunk_lists);  index++ ) */
  for( index = 0;  index < HA_MAX_INDEX;  index++ )
  {
    chunk = as->free_chunk_lists[index];
    /* HaChunkCheckMagic(chunk); -- no! chunk may be NULL */
    HaChunkListFree(chunk);
  }

  /* free the chunks from the private arena */
  chunk = as->private_arena->first_busy_chunk;
  /* HaChunkCheckMagic(chunk); -- no! chunk may be NULL */
  HaChunkListFree(chunk);
}


/*****************************************************************************/
/*                                                                           */
/*  HA_JUMP_ENV HaJumpEnvMake(jmp_buf *env, HA_ARENA a)                      */
/*                                                                           */
/*  Make a new jump environment object for as.                               */
/*                                                                           */
/*****************************************************************************/

static HA_JUMP_ENV HaJumpEnvMake(jmp_buf *env, HA_ARENA_SET as)
{
  HA_JUMP_ENV res;
  HaArenaSetCheckMagic(as);
  if( HaArrayCount(as->free_jump_envs) > 0 )
  {
    /* get a jump environment object from as's free list */
    res = HaArrayLastAndDelete(as->free_jump_envs);
    HaArrayClear(res->arenas);
  }
  else
  {
    /* make a new jump environment object */
    HaMake(res, as->private_arena);
    HaArrayInit(res->arenas, as->private_arena);
  }
  res->env = env;
  HaArenaSetCheckMagic(as);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetJmpEnvBegin(HA_ARENA_SET as, jmp_buf *env)                */
/*                                                                           */
/*  Begin a longjmp environment.                                             */
/*                                                                           */
/*****************************************************************************/

void HaArenaSetJmpEnvBegin(HA_ARENA_SET as, jmp_buf *env)
{
  HA_JUMP_ENV jump_env;
  HaArenaSetCheckMagic(as);
  jump_env = HaJumpEnvMake(env, as);
  HaArrayAddLast(as->jump_envs, jump_env);
  HaArenaSetCheckMagic(as);
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetJmpEnvEnd(HA_ARENA_SET as)                                */
/*                                                                           */
/*  End a longjmp environment.                                               */
/*                                                                           */
/*****************************************************************************/

void HaArenaSetJmpEnvEnd(HA_ARENA_SET as)
{
  int count;  HA_JUMP_ENV jump_env;
  HaArenaSetCheckMagic(as);
  count = HaArrayCount(as->jump_envs);
  HnAssert(count > 0, "HaArenaSetJmpEnvEnd: no matching HaArenaSetJmpEnvBegin");
  jump_env = HaArrayLastAndDelete(as->jump_envs);
  HnAssert(HaArrayCount(jump_env->arenas) == 0,
    "HaArenaSetJmpEnvEnd: %d arenas created since the matching "
    "HaArenaSetJmpEnvBegin are still busy", HaArrayCount(jump_env->arenas));
  HaArrayAddLast(as->free_jump_envs, jump_env);
  HaArenaSetCheckMagic(as);
}


/*****************************************************************************/
/*                                                                           */
/*  void HaArenaSetLimitMemory(HA_ARENA_SET as, size_t limit)                */
/*                                                                           */
/*  Limit the amount of memory allocated by the arenas of as to at most      */
/*  limit bytes.  A limit of 0 means "no limit".                             */
/*                                                                           */
/*****************************************************************************/

void HaArenaSetLimitMemory(HA_ARENA_SET as, size_t limit)
{
  HaArenaSetCheckMagic(as);
  as->max_words = HaBytesToWords(limit);
  HaArenaSetCheckMagic(as);
}


/*****************************************************************************/
/*                                                                           */
/*  int HaChunkListLength(HA_CHUNK chunk)                                    */
/*                                                                           */
/*  Return the length of the list of chunks beginning with chunk.            */
/*                                                                           */
/*****************************************************************************/

static int HaChunkListLength(HA_CHUNK chunk)
{
  int res;
  res = 0;
  for( ;  chunk != NULL;  chunk = chunk->next_chunk )
  {
    HaChunkCheckMagic(chunk);
    res++;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/* void HaArenaSetDebug(HA_ARENA_SET as, int verbosity, int indent,          */
/*   FILE *fp)                                                               */
/*                                                                           */
/*  Debug print of arena set as onto fp with the given verbosity and indent. */
/*                                                                           */
/*****************************************************************************/

void HaArenaSetDebug(HA_ARENA_SET as, int verbosity, int indent, FILE *fp)
{
  HA_CHUNK chunk;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Arena Set %p:\n", indent, "", (void *) as);
    fprintf(fp, "%*s  free arenas    %12d%s\n", indent, "",
      HaArrayCount(as->free_arenas), HaArrayCount(as->free_arenas) > 0 &&
        HaArrayLast(as->free_arenas) == NULL ? " (last is NULL)" : "");
    fprintf(fp, "%*s  busy arenas    %12d\n", indent, "",as->busy_arenas_count);
    fprintf(fp, "%*s  jmp_bufps      %12d\n", indent, "",
      HaArrayCount(as->jump_envs));
    fprintf(fp, "%*s  busy words     %12ld\n", indent, "", as->curr_words);
    fprintf(fp, "%*s  max busy words %12ld\n", indent, "", as->max_words);
    /* HaArrayForEach(as->free_chunk_lists, chunk, i) */
    for( i = 0;  i < HA_MAX_INDEX;  i++ )
    {
      chunk = as->free_chunk_lists[i];
      fprintf(fp, "%*s  free_chunk_list[%d] has length %d\n", indent, "",
	i, HaChunkListLength(chunk));
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "arrays"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int HaArrayUniqSort(void *base, size_t nmemb, size_t size,               */
/*      int(*compar)(const void *, const void *))                            */
/*                                                                           */
/*  The parameters of HaArrayUniqSort are the same as for qsort and have the */
/*  same meaning.  This function first sorts using qsort, then uniqueifies   */
/*  the result array by removing each element which compares equal to the    */
/*  preceding element.  It returns the final number of elements.             */
/*                                                                           */
/*****************************************************************************/

int HaArrayUniqSort(void *base, size_t nmemb, size_t size,
    int(*compar)(const void *, const void *))
{
  int i, j;
  if( nmemb > 0 )
  {
    qsort(base, nmemb, size, compar);
    i = 0;
    for( j = 1;  j < nmemb;  j++ )
      if( compar(&((char *) base)[i * size], &((char *) base)[j * size]) != 0 )
      {
	i++;
	if( i != j )
	  memcpy(&((char *) base)[i * size], &((char *) base)[j * size], size);
      }
    return i + 1;
  }
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool HaArrayImplFind(char *a, char *target, size_t elem_size,            */
/*    int count, int *pos)                                                   */
/*                                                                           */
/*  Scan a in chunks of size elem_size, looking for *target.                 */
/*                                                                           */
/*****************************************************************************/

bool HaArrayImplFind(char *a, char *target, size_t elem_size,
  int count, int *pos)
{
  char *p;
  for( *pos = 0, p = a;  *pos < count;  (*pos)++, p += elem_size )
    if( memcmp(p, target, elem_size) == 0 )
      return true;
  return false;
}
