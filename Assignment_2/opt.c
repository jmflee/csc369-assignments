#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"
#include "pagetable.h"

extern struct frame *coremap;

struct node {
	addr_t vaddr;
	struct node* next;
};

struct node* head;
struct node* curr;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	
	int i, frame, dist;
	int highest = 0;
	for (i = 0; i < memsize; i++) { // cycles frames in memory
        if (coremap[i].vaddr) { // Skips vaddresses that don't exist
            struct node* p = curr->next; // p is the probable next occurence of the frame
            dist = 0; // Reset entry of distance from last time
            // Distances keeps incrementing while it doesn't find another occurence of the vaddress
			while ((p && p->vaddr != coremap[i].vaddr) && dist <= highest) {
				dist++;
                if (!p) { // Frame will never come back
                    return i; // So this frame will be evicted
                }
				p=p->next; // Next frame
			}
        }	
        if (dist >= highest) {
            highest = dist; // World record highest distance
            frame = i; // To remove
        }
	}
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {	
	curr = curr->next; //Update current when a frame is referrenced
    if (!curr) {  //  Round robin when reaching the end
        curr = head;
    }
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	char traces[MAXLINE];
	FILE* file;
	char type;
	head = NULL;
	curr = NULL;
	addr_t vaddr;

	// Opening tracefile for reading similar from sim.c
	if (tracefile != NULL) {
		if((file= fopen(tracefile, "r")) == NULL) {
			perror("Error opening tracefile:");
			exit(1);
		}
	}
	while (fgets(traces, MAXLINE, file) != NULL) {
		if (traces[0] != '=') {
			sscanf(traces, "%c %lx", &type, &vaddr);
			struct node *new_t = malloc(sizeof(struct node));
			new_t->vaddr = vaddr;
			new_t->next = NULL;
			if (head == NULL) {
				head = new_t;
				curr = new_t;
			} else {
				curr->next = new_t;
				curr = curr->next;
			}
		}
	}
	// Set current node to start at head;
	curr = head;
	fclose(file);
}

