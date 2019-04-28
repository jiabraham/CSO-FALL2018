#include <assert.h>

// Swap the values of the integer variables pointed to by p1 and
// p2.  After the function returns, the variable pointed to by p1
// should have the value once pointed to by p2, and vice-versa.
//
// You may use a temporary variable or do an XOR swap.
void
swap(int *p1, int *p2)
{
	//assert(p1 != 0);
	//assert(p2 != 0);

	// TODO: Your code here.
        //Temporary variable
	int temp = *p1;
        //Set first pointer equal to the second one
	*p1 = *p2;
        //Set second pointer equal to the temp variable which held
        //the inputted first pointer
	*p2 = temp;

}

// bubble_sort implements the bubble sort algorithm.
// https://en.wikipedia.org/wiki/Bubble_sort
// It is given an integer array "arr" of "n" elements. 
// Upon return, the integer array should contain the sorted numbers
// in increasing order. 
// You should use the previous swap function when implementing bubble_sort
void
bubble_sort(int *arr, int n)
{
        // TODO: Your code here.
        //integer that is essentially a boolean for this function
        int inorder = 0;

        //Loop goes through the array of integers mulitple times if necessary
        while (inorder != 1) {
                //First for loop goes through and swaps all elements 
                //out of order that are next to each other
        	for (int i = 0; i < n - 1; i++) {
                        //check to see if elements aren't increasing
        		if (*(arr + i) > *(arr + i + 1)) {
                                //use of swap function
        			swap(arr + i, arr + i + 1);
        		}
        	}
                //Counts how many times numbers are still out of order
        	int mistakes = 0;
                //Loop goes through array to check if out of order
        	for (int i = 0; i < n - 1; i++) {
                        //Check if elements are increasing
        		if (*(arr + i) > *(arr + i + 1)) {
                                //increment mistakes if elements are out of order
        			mistakes++;
        		}
        	}
                //if all elements are in order, this breaks out of the while loop
                if (mistakes == 0) {
                        inorder = 1;
                }
        }
        return;
}
