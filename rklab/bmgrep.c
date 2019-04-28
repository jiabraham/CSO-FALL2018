#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "bmgrep.h"


#define MAX(a,b) ((a > b) ? a : b) 

/* allocate, initialize and return the delta1 array according to the BM paper. 
 * The delta1 array should have 128 integer elements, one for each valid ASCII character.
 * delta1[c] is set the distance from the right where c is found in the pattern.
 * if c is not found in the pattern, x = strlen(pattern).
 */
int*
init_delta1(const char *pattern) {
	// Your code here. Do not return NULL

	int* delta1 = malloc(128 * sizeof(int));
	for (int i = 0; i < 128; i++) delta1[i] = -1;

	const char *tmp = pattern;
	int patlen = strlen(pattern);
	int index = 0;
	while (*tmp != '\0') {
		delta1[*tmp] = patlen - index -1;
		tmp++;
		index++;
	}
	for (int i = 0; i < 128; i++) {
		if (delta1[i] == -1) {
			delta1[i] = patlen;
		}
	}
	return delta1; 
}

/* Let plen = strlen(pattern).
 * Return the largest position where
 * the terminal substring pattern[j+1..plen] re-occurs and is not 
 * preceded by the character pattern[j].
 *
 * The BM paper describes how to calculate this value, aka rpr(j),  
 * in Sec 4 (the last two paragraphs).
 * Note that in your implementation, positions start from 0. 
 * By contrast, in the BM paper, positions start from 1.
 */
int find_rpr(const char *pattern, int j) {
	// Your code here. Do not return strlen(pattern), which is never a valid return value.
	int patlen = strlen(pattern);
	int matchpos = -1;
	if (j < 0 || j >= patlen) return -1;
	// Should this return 0??
	if (j == patlen-1) return 1;
	for (int i=j+1; i < patlen; i++) {
		for (int k=0; k < j; k++) {
			if (pattern[k] == pattern[i]) {
				if (matchpos == -1) matchpos = k;
			} else {
				matchpos = -1;
			}
		}
	}
	return matchpos;
}

/* Allocate, initialize and return the delta2 array according to the BM paper (page 765).
 * The delta2 array should contain strlen(pattern) integer elements, 
 * one corresponding to each position in the pattern. 
 * Let plen = strlen(pattern).
 * delta2[j] = plen - find_rpr(pattern,j) 
 * Note that the above formula uses "plen" instead of "patlen+1" as in the BM paper.
 * This is because string positions range from 1 to patlen in the BM paper,
 * however, in C, string positions range from 0 to strlen(pattern)-1.
 * Also note that delta2[plen-1] should always be 1.
 */
int*
init_delta2(const char *pattern) {
	// Your code here. Do not return NULL
	//Did not finish,some kind of bug here
	int plen = strlen(pattern);
	printf("delta2: plen = %d\n", plen);
	int* delta2 = malloc(plen * sizeof(int));
	for (int i = 0; i < plen; i++) {
		delta2[i] = plen - find_rpr(pattern,i);
		printf("delta2 %d = %d\n", i, delta2[i]);
		// put prints to examine delta2
	}
	return delta2;
}


/* Return the position of the first occurance of the pattern in doc using Boyer-Moore algorithm.
 * If the pattern is not found, return -1.
 * You need to use helper functions (init_delta1, init_delta2) and follow the algorithm in Sec 4.
 * Note: do not use any goto statements in your code!
 *
 * Additionally, return the number of alignments you've done during the matching process by writing to
 * the deferenced pointer n_aligns.
 * The number of alignments is the number of times you've advanced the "i" pointer to the right, 
 * including its initialization.
 * For example, the number of alignments done for the example on page 764 is 5.
 */
int 
boyer_moore(const char *pattern, const char *doc, int *n_aligns) {
	int stringlen = strlen(doc);
	int patlen = strlen(pattern);
	int *delta1 = init_delta1(pattern);
	int *delta2 = init_delta2(pattern);
	int i = patlen;
	if (i > stringlen) return -1;
	while (i < stringlen) {
		bool found = true;
		int j = patlen;
		while (j != 0) {
			(*n_aligns)++;
			if (doc[i] == pattern[j]) {
				j--;
				i--;
			}
			else {
				found = false;
				break;
			}
		}
	
		if (found) return i;

		i = i + MAX(delta1[doc[i]], delta2[j]);

	}
	return -1;



}

