/* NYU Computer Systems Organization Lab 2
 * Rabin-Karp Substring Matching
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include "rkgrep.h"
#include "bloom.h"

#define PRIME 961748941

// calculate modulo addition, i.e. (a+b) % PRIME
long long
madd(long long a, long long b)
{
	return (a + b) % PRIME;
}

// calculate modulo substraction, i.e. (a-b) % PRIME
long long
msub(long long a, long long b)
{
	return (a>b)?(a-b):(a+PRIME-b);
}

// calculate modulo multiplication, i.e. (a*b) % PRIME
long long
mmul(long long a, long long b)
{
	return (a*b) % PRIME;
}

//Calculate the length of a string(if we don't want to use the standard strlen function)
unsigned int
strlength(const char *pattern) {

	const char *tmp = pattern;
	unsigned int count = 0;

	while (*tmp != '\0') {
		count++;
		tmp++;
	}

	return count;
}
/* naive_substring_match returns number of positions in the document where
 * the pattern has been found.  In addition, it stores the first position 
 * where the pattern is found in the variable pointed to by first_match_ind
 *
 * Both its arguments "pattern" and "doc" are null-terminated C strings.
 */
int
naive_substring_match(const char *pattern, const char *doc, int *first_match_ind)
{
	//count all the times pattern matches a substring of doc
	int count = 0;
	
	//temporary pointer to iterate through every index of doc
	const char* doctmp = doc;
	
	//index to supply the value for *first_match_index
	int index = 0;
	
	//Initialized first match index to later check if it is indeed the first match
	*first_match_ind = -1;

	//while loop to iterate over doctmp
	while (*doctmp != '\0') {
		
		//first_location reset 
		int first_location = index;
		
		//boolean to assure that the full pattern matched a substring in doc
		bool found = false;
		
		//temporary pointer to iterate over pattern
		const char* pattmp = pattern;
		
		//temporary pointer to start compare with *pattmp
		const char* docsub = doctmp;

		//loop that will iterate over the pattern
		while (*pattmp != '\0') {

			//check to make sure that every character matches
			if (*pattmp == *docsub) {
				
				//increment both temporary pointers
				pattmp++;
				docsub++;

				//check to make sure the last char of doc doesn't cause issues
				if (*docsub == '\0' && pattmp != '\0') {
					
					//no match means found is false
					found = false;

					//break out of inner loop
					break;
				}
				//otherwise found is true with characters matching
				found = true;
			}
			else {

				//if only some characters match, match must be false
				found = false;

				//if only some characters match, this will break inner loop
				break;
			}
		}
		//check to see if a substring of doc matches pattern
		if (found) {
			//check to see if it is the first match found in doc
			if (*first_match_ind == -1) {
				*first_match_ind = first_location;
				//printf("%d ")
			}
			//if it is the first match or not, still the count of matches needs to be incremented
			count++;
		}	
		//increment both index of doc and doctmp pointer;
		index++;
		doctmp++;
	}
	//return the number of matches found
	return count;
}	

/* initialize the Rabin-karp hash computation by calculating 
 * and returning the RK hash over a charbuf of m characters, 
 * i.e. The return value should be 
 * 256^(m-1)*charbuf[0] + 256^(m-2)*charbuf[1] + ... + charbuf[m-1],
 * where 256^(m-1) means 256 raised to the power m-1.
 * Note that all operations *, +, - are modulo arithematic, so you 
 * should use the provided functions mmul, madd, msub.
 * (We use "long long" to represent an RK hash)
 */
long long
rkhash_init(const char *charbuf, int m, long long *h)
{
	//Hashret will hold the RK hash
	long long hashret = 0;
	
	//rkval is the value to be passed into the pointer h
	long long rkval = 1;

	//Foor loop established to iteratively calculate the hash value of a charbuf
	//Calculated backwards from last character to the first
	//Started at m-1 so i does not need to be adjusted within the function
	for (int i=m-1; i>=0; --i) {

		//Set hashret equal to modulo sum of previous hashret and the next character hash value
		hashret = madd(hashret, mmul(rkval, charbuf[i]));

		//rkvalue is modulo multiplied by 256 for each of the m characters
		rkval = mmul(rkval, 256);
	}
	//set the value h is pointing to to rkval
	*h = rkval;

	//return the hash value 
	return hashret;

}


/* Given the rabin-karp hash value (curr_hash) over substring Y[i],Y[i+1],...,Y[i+m-1]
 * calculate the hash value over Y[i+1],Y[i+2],...,Y[i+m] = curr_hash * 256 - leftmost * h + rightmost
 * where h is 256 raised to the power m (and given as an argument).  
 * Note that *,-,+ refers to modular arithematic so you should use mmul, msub, madd.
 */
long long 
rkhash_next(long long curr_hash, long long h, char leftmost, char rightmost)
{
	//This calculates the next hash value of a moving filter
	//(e.g. given hash of i to i + m - 1, calculate hash of i + 1 to i + m)
	long long hashret = madd(msub(mmul(256, curr_hash), mmul(leftmost, h)), rightmost);
	//Return the next hash
	return hashret;
}

/* rk_substring_match returns the number of positions in the document "doc" where
 * the "pattern" has been found, using the Rabin-karp substring matching algorithm.
 * Both pattern and doc are null-terminated C strings. The function also stores
 * the first position where pattern is found in the int variable pointed to by first_match_ind
 *
 * Note: You should implement the Rabin-Karp algorithm by completing the 
 * rkhash_init and rkhash_next functions and then use them here.
*/
int
rk_substring_match(const char *pattern, const char *doc, int *first_match_ind)
{
	//count all the times pattern matches a substring of doc
	int count = 0;
	
	//temporary pointer to iterate through every index of doc
	const char* doctmp = doc;
	
	//index to supply the value for *first_match_index
	int index = 0;
	
	//Initialized first match index to later check if it is indeed the first match
	*first_match_ind = -1;

	//strlen function gives the length of the pattern that gives indexes which need to be checked
	int pattern_length = strlength(pattern);

	//strlen function gives the length of the document which along with pattern_legth gives indexes
	int doc_length = strlength(doc);

	//If somehow, someway the pattern length we need to match is greater 
	//than the doc length, this check returns 0 because the input is wrong or incompatible
	if (pattern_length > doc_length) {
		return 0;
	}

	long long h;
	long long pattern_hash = rkhash_init(pattern, pattern_length, &h);
	long long doc_hash = rkhash_init(doc, pattern_length, &h);

	//while loop to iterate over doctmp
	while (index <= doc_length - pattern_length) {
		
		if (pattern_hash == doc_hash) {

			//double check to make sure they match
			//boolean to assure that the full pattern matched a substring in doc
			bool found = false;
		
			//temporary pointer to iterate over pattern
			const char* pattmp = pattern;
		
			//temporary pointer to start compare with *pattmp
			const char* docsub = doctmp;

			//loop that will iterate over the pattern
			while (*pattmp != '\0') {

				//check to make sure that every character matches
				if (*pattmp == *docsub) {
				
					//increment both temporary pointers
					pattmp++;
					docsub++;

					//otherwise found is true with characters matching
					found = true;
				}
				else {

					//if only some characters match, match must be false
					found = false;

					//if only some characters match, this will break inner loop
					break;
				}
			}
			//check to see if a substring of doc matches pattern
			if (found) {
				//check to see if it is the first match found in doc
				if (*first_match_ind == -1) {
					*first_match_ind = index;
				}
				//if it is the first match or not, still the count of matches needs to be incremented
				count++;
			}	
		}
		//This increments the doc_hash to be compared with
		doc_hash = rkhash_next(doc_hash, h, *doctmp, doctmp[pattern_length]);

		//increment both index of doc and doctmp pointer;
		index++;
		doctmp++;
	}
	//return the number of matches found
	return count;
    
	return 0;
}


/* rk_create_doc_bloom returns a pointer to a newly created bloom_filter. 
 * The new bloom filter is populated with all n-m+1 rabin-karp hashes for 
 * all the substrings of length m in "doc".
 * Hint: use the provided bloom_init() and your implementation of bloom_add() here.
 */
bloom_filter *
rk_create_doc_bloom(int m, const char *doc, int bloom_size)
{
	//temporary pointer to iterate through every index of doc
	const char* doctmp = doc;

	//Declare and initialize pointer to bloom filter
	bloom_filter *bf = bloom_init(bloom_size);

	//Declare h which is set in rkhash_init
	long long h;
	
	//Declare and initialize doc_hash to be calculated by rkhash_init
	long long doc_hash = rkhash_init(doc, m, &h);

	//Blook add sets the bitmap
	bloom_add(bf, doc_hash);

	//Declare index to iteratively run through doc
	int index = 1;

	//Use strlen to calculate the length of doc
	int doc_length = strlength(doc);

	//while loop to iterate over doctmp
	//Only need to check up to (doc_length - m) because indexes after that don't contain enough letters to have a match
	while (index <= doc_length - m) {
		//Set doc_hash to the next hash value of m elements
		doc_hash = rkhash_next(doc_hash, h, *doctmp, doctmp[m]);

		//Adds doc_hash into the bloom filter
		bloom_add(bf, doc_hash);
		//increment both index of doc and doctmp pointer;
		index++;
		doctmp++;
	}
	//return pointer pointing to bloom filter
	return bf;
}

/* rk_substring_match_using_bloom returns the total number of positions where "pattern" 
 * is found in "doc".  It performs the matching by first checking against the 
 * pre-populated bloom filter "bf" (which has been created by rk_create_doc_bloom on "doc")
 * If the pattern is not found in "bf", then the function immediately returns 0.
 * Otherwise, the function invokes rk_substring_match() function to find "pattern" in "doc".
*/
int
rk_substring_match_using_bloom(const char *pattern, const char *doc, bloom_filter *bf, int *first_match_ind)
{
    /* Your code here */
    //Calculates the length of the pattern the program is trying to find
	int pattern_length = strlength(pattern);

	//h simply needs to be declared and initialized, it's later set in rkhash_init
    long long h = 0;

    //Calulates the hash value of the pattern to be matched within doc
	long long pattern_hash = rkhash_init(pattern, pattern_length, &h);

	//Check to see if a hash matches some pattern in doc
	if (bloom_query(bf, pattern_hash)) {
		//Run the naive to make sure that the match is really a match and not just a coincidence
		return rk_substring_match(pattern, doc, first_match_ind);
	}
	//If bloom query returns false, then no matches are found and 0 is returned
    return 0;
}
