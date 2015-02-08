/* malloc.c - a malloc(3) and free(3) implementation.
 *
 * This is an extremely simple implementation of the malloc(3) and free(3)
 * library functions, responsible for allocatiing memory from the heap to
 * a process.
 *
 * This implementation uses a very basic algorithm as described in
 * "The Linux Programming Interface" book, maintaining a doubly-linked list
 * of free memory blocks to reduce the number of brk(2) system calls.
 * Contiguous blocks of memory may be given back to the system if they
 * are larger than _MALLOC_MAX_FREE_BLK (which defaults to 128KB). Note that
 * critical issues such as thread-safety are not at all considered here, since
 * that completeness is not the goal here, obviously. This implementation
 * will be enough only for the most basic usages, and has a few glitches
 * on certain undetermined occasions.
 *
 * You can test this implementation using the LD_PRELOAD environment
 * variable to override the malloc implementation with the one provided
 * in this file.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE
#define _POSIX_SOURCE

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#ifdef _MALLOC_DEBUG
#  include <stdio.h>
#  include <stdarg.h>
#  include <string.h>
#endif

#ifndef _MALLOC_MAX_FREE_BLK
#  define _MALLOC_MAX_FREE_BLK (128 * 1024)
#endif

#define _MALLOC_MAX_DEBUG_STR  (1024)
#define _MALLOC_HEADER_SIZE    (sizeof(size_t))
#define _MALLOC_POINTER_SIZE   (sizeof(void *))

static char *_free_list = NULL;

#ifdef _MALLOC_DEBUG
static void
debug(const char *format, ...) {
	va_list args;
	va_start(args, format);

	char prefixed_format[_MALLOC_MAX_DEBUG_STR];
	snprintf(prefixed_format, _MALLOC_MAX_DEBUG_STR, "[malloc] %s\n", format);

	vfprintf(stderr, prefixed_format, args);
	va_end(args);
}
#else
static void
debug(__attribute__((unused)) const char *format, ...) {}
#endif

/* adds size information at the start of a block */
static void
write_size(void *ptr, size_t size) {
	size_t *sizep = ptr;
	*sizep = size;
}

/* reads the size of a block, which is located as metadata in its first bytes */
static size_t
read_size(void *ptr) {
	size_t *sizep = ptr;
	return *sizep;
}

/* what follows are a set of helper functions to read and write the previous and
 * next free block pointers in the free list. */

static void
set_previous_free_block(void *ptr, void *previous_ptr) {
	/* the previous block pointer is located right after the size information */
	char *p = ptr;
	void **previous_block_info;

	p += _MALLOC_HEADER_SIZE;
	previous_block_info = (void **) p;

	*previous_block_info = previous_ptr;
}

static void *
get_previous_free_block(void *ptr) {
	char *p = ptr;
	void **previous_block_info;

	p += _MALLOC_HEADER_SIZE;
	previous_block_info = (void **) p;

	return *previous_block_info;
}

static void
set_next_free_block(void *ptr, void *next_ptr) {
	/* the next block pointer is located right after the previous block pointer */
	char *p = ptr;
	void **next_block_info;

	p += _MALLOC_HEADER_SIZE + _MALLOC_POINTER_SIZE;
	next_block_info = (void **) p;

	*next_block_info = next_ptr;
}

static void *
get_next_free_block(void *ptr) {
	char *p = ptr;
	void **next_block_info;

	p += _MALLOC_HEADER_SIZE + _MALLOC_POINTER_SIZE;
	next_block_info = (void **) p;

	return *next_block_info;
}

/* this calculates and returns the base address of a block given by the user to
 * the `free` function. The block metadata is "hidden" from the user in calls
 * to `malloc`, and must be recovered here */
static void *
get_base_address(void *ptr) {
	size_t *sizep = ptr;
	return (sizep - 1);
}

/* the last free block is the one whose end is adjacent to the current program
 * break. When expanding the program break, it is useful to take into account
 * what is the condition of this block */
static void *
last_free_block() {
	void *curr, *prev;
	curr = _free_list;

	while (curr) {
		prev = curr;
		curr = get_next_free_block(prev);
	}

	return prev;
}

/* slice: takes a block of memory pointed by ptr and slice it, returning a
 * block of `size` bytes. `ptr` is required to be larger than the requested
 * size. All links in the free list are maintained and the block of memory
 * returned already skips size information */
static void *
slice(void *ptr, size_t size) {
	void *previous_ptr = get_previous_free_block(ptr),
	     *next_ptr     = get_next_free_block(ptr);

	size_t original_size = read_size(ptr);
	if (original_size <= size)
		return NULL;

	char *p = ptr;
	char *base = ptr;
	p += _MALLOC_HEADER_SIZE + size;
	if (previous_ptr)
		set_next_free_block(previous_ptr, p);
	else
		_free_list = p;

	if (next_ptr)
		set_previous_free_block(next_ptr, p);

	write_size(ptr, size);
	write_size(p, original_size - size - _MALLOC_HEADER_SIZE);
	set_previous_free_block(p, previous_ptr);
	set_next_free_block(p, next_ptr);

	return (base + _MALLOC_HEADER_SIZE);
}

/* returns the address of the last position of the given memory block */
static void *
end_address(void *ptr) {
	char *p = ptr;
	return (p + _MALLOC_HEADER_SIZE + read_size(ptr));
}

/* checks if the two given memory block pointers are continuous */
static int
continuous(void *ptr1, void *ptr2) {
	return end_address(ptr1) == ptr2;
}

/* coalesces two memory blocks (assumed to be continuous) and updates related
 * metadata */
static void
coalesce(void *ptr1, void *ptr2) {
	size_t new_size = read_size(ptr1) + _MALLOC_HEADER_SIZE + read_size(ptr2);
	write_size(ptr1, new_size);
}

/* checks if the last free block in the free memory list is larger than the
 * allowed _MALLOC_MAX_FREE_BLK, in which case memory is given back to the
 * system, reducing the process' memory footprint */
static void
check_footprint() {
	void *last = last_free_block(),
	     *prev = get_previous_free_block(last);

	size_t last_size = read_size(last);

	if (last_size >= _MALLOC_MAX_FREE_BLK) {
		if (prev)
			set_next_free_block(prev, NULL);

		sbrk(-1 * (last_size + _MALLOC_HEADER_SIZE));
	}
}

static void
print_free_list() {
#ifdef _MALLOC_DEBUG
	void *p = _free_list;
	char list[_MALLOC_MAX_DEBUG_STR], node[_MALLOC_MAX_DEBUG_STR];

	snprintf(list, _MALLOC_MAX_DEBUG_STR, "FL:");

	while (p) {
		strncat(list, " -> ", _MALLOC_MAX_DEBUG_STR);
		snprintf(node, _MALLOC_MAX_DEBUG_STR, "[%p S=%ld P=%p N=%p]",
				p, read_size(p), get_previous_free_block(p), get_next_free_block(p));
		strncat(list, node, _MALLOC_MAX_DEBUG_STR);

		p = get_next_free_block(p);
	}

	debug(list);
#endif
}

void *
malloc(size_t size) {
	char *breakp;

	/* SUSv3 allows an implementation to return either NULL or a small
	 * memory block in this situation. We follow the latter, which is
	 * the behavior implemented on Linux */
	if (size == 0)
		size = 1;

	debug("Malloc request of size %ld", (long) size);

	if (!_free_list) {
		debug("No free list found, creating one of size %ld", (long) 2 * size);

		/* if there is no free list (i.e., the first call of the function
		 * in the process life), we allocate twice as much memory as
		 * requested as an attempt to avoid further system calls */
		breakp = sbrk(2 * size + _MALLOC_HEADER_SIZE);
		if (breakp == (void *) -1)
			return NULL;

		_free_list = breakp;
		write_size(_free_list, 2 * size);
		set_previous_free_block(_free_list, NULL);
		set_next_free_block(_free_list, NULL);
	}

	print_free_list();

	/* look for a suitable free block of memory to be used. Note that, for
	 * simplicity sake, the search uses a first-fit fashion algorithm, returning
	 * the first block of memory large enough to fulfil the request. */
	char *p = _free_list;
	while (p) {
		if (read_size(p) > size + _MALLOC_HEADER_SIZE)
			return slice(p, size);

		p = get_next_free_block(p);
	}


	/* if we get to this point, it means there was no large enough memory block
	 * that could fulfil the request. In this case, we move the program break,
	 * increasing the memory footprint of the process */
	void *last = last_free_block();
	size_t last_size = read_size(last),
	       break_increase = 2 * size + _MALLOC_HEADER_SIZE;

	debug("No large enough free block, expanding program break by %ld bytes.", (long) break_increase);
	if (sbrk(break_increase) == (void *) -1) {
		debug("Fail to increase program break");
		return NULL;
	}

	write_size(last, last_size + break_increase);
	return slice(last, size);
}

void
free(void *ptr) {
	/* SUSv3 allows the pointer given to `free` to be NULL, in which case
	 * nothing should be done */
	if (!ptr)
		return;

	/* if the free list was not created yet, this means that the memory block
	 * passed to this function was not obtained through malloc, and indicates
	 * memory corruption. We indicate that by sending the current process
	 * a SIGSEGV signal */
	if (!_free_list) {
		debug("Memory block not allocated by malloc");
		kill(getpid(), SIGSEGV);
	}

	void *base_address = get_base_address(ptr), *p;
	size_t blk_size = read_size(base_address);

	debug("Free request for block of size %ld", (long) blk_size);
	print_free_list();

	/* we must find where to insert the given memory block in the existing
	 * free list, keeping in mind that the list should be in the same ordered
	 * that the actual memory is (virtual memory-wise, of course). */
	void *curr = _free_list, *prev = NULL;
	while (curr && curr < base_address) {
		prev = curr;
		curr = get_next_free_block(prev);
	}

	/* here, we either got to the end of the free list, or found out the
	 * blocks between which the new block should be inserted */
	if (prev) {
		if (curr) {
			/* prev -> ptr -> curr */
			debug("Freed block being inserted in the middle of the free list");

			if (continuous(prev, base_address)) {
				coalesce(prev, base_address);
			} else if (continuous(base_address, curr)) {
				coalesce(base_address, curr);

				set_next_free_block(prev, base_address);
				set_previous_free_block(base_address, prev);
				set_next_free_block(base_address, get_next_free_block(curr));

				check_footprint();
			} else {
				set_next_free_block(prev, base_address);
				set_previous_free_block(curr, base_address);

				set_previous_free_block(base_address, prev);
				set_next_free_block(base_address, curr);
			}
		} else {
			/* prev -> ptr */
			debug("Appending freed block at the end of the list");
			if (continuous(prev, base_address)) {
				coalesce(prev, base_address);
				check_footprint();

			} else {
				set_next_free_block(prev, base_address);
				set_previous_free_block(base_address, prev);
				set_next_free_block(base_address, NULL);
			}
		}
	} else {
		/* free_list = ptr -> curr */
		debug("Freed block should be the new free list head pointer");
		if (continuous(base_address, curr)) {
			debug("actually joined with current head");
			coalesce(base_address, curr);

			p = get_next_free_block(curr);
			if (p)
				set_previous_free_block(p, base_address);

			set_previous_free_block(base_address, NULL);
			set_next_free_block(base_address, p);

			check_footprint();

		} else {
			set_previous_free_block(curr, base_address);
			set_previous_free_block(base_address, NULL);
			set_next_free_block(base_address, curr);
		}

		_free_list = base_address;
	}
}
