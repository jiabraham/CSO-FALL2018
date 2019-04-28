//Implement a C function called ex1 such that it achieves 
//the same functionality as the machine code of objs/ex1_sol.o
//Note: you need to figure out ex1()'s function signature yourself; 
//the signature is not void ex1()
  
#include <assert.h>

//Function takes in 3 integer arguments
//Checks if sum of first two equal the third
//returns 1 for equality and 0 for inequality
int ex1(int a, int b, int c) {
	//increment one of the first arguments by the other
	b += a;
	//sum of the first two args same as third
	if (c == b) return 1;
	//return 0 if comparison returns false
	return 0;
}
