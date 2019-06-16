#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int time;
/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
    pgtbl_entry_t *position;
    while (1) {
        position = coremap[time].pte;
        if (!(position->frame & PG_REF)) {
            break; // Break and return the PFN if it is referenced
        }
        position->frame &= ~PG_REF; // The frame is dereferenced
        time++;
        if (time >= memsize) { 
            time = 0; // Round robin the clock
        }
    }
	return time;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	p->frame |= PG_REF; 
    return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
    time = 0;
}
