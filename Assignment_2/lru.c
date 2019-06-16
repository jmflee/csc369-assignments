#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
extern int memsize;

extern int debug;

extern struct frame *coremap; 

int usetimes;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
		
	int frame; // Frame number to evict
	int i = 0;
	int mintime = usetimes; // Set minimum time to current usetimes

	// Find the frame with smallest usetimes
	while (i < memsize) {
		if (mintime > coremap[i].reftimes) {
			// Replace minimum time to current frames time stamp
			mintime = coremap[i].reftimes;
			frame = i;
		}
		i++;
	}
	
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	// Increment use times when a frame is refferenced
	usetimes++;
	// Make frame timestamp to current usetimes
	int i=0;
	while (coremap[i].pte != NULL) {
		if (coremap[i].pte->frame == p->frame) {
			coremap[i].reftimes = usetimes;
			break;
		}
		i++;
	}
	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	// Set usetimes to 0 at start
	usetimes = 0;
}
