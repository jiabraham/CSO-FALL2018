//Implement a C function called ex4 such that it achieves 
//the same functionality as the machine code of objs/ex4_sol.o
//Note: you need to figure out ex4()'s function signature yourself; 
//the signature is not void ex4()
  
#include <assert.h>
//Returns the sum of the first rd fibonacci numbers
int
ex4(int rd) {
	//return arg if 0 or 1 (or negative number if for some reason called first)
	if (rd < 2) return rd;
	//return the function call sum of the previous 2 numbers
	return (ex4(rd - 2) + ex4(rd - 1));
	 
}
