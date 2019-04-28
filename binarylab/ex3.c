//Implement a C function called ex3 such that it achieves 
//the same functionality as the machine code of objs/ex3_sol.o
//Note: you need to figure out ex3()'s function signature yourself; 
//the signature is not void ex3()
  
#include <assert.h>
#include <string.h>
//This function takes two char pointers and an integer b
//In each char array, it swaps the first b chars of the first pointer to the first b chars of the second
void
ex3(char* rd, char* rs, int b) {
	//keeping track of the index at both the rd pointer and rd pointer
	int index = 0;
	//while loop to cycle through the first b chars
	while (index < b) {
		//temporary char for swap
		char tmp = rd[index];
		//set the first pointer char equal to the second pointer char
		rd[index] = rs[index];
		//set the second pointer char equal to the tmp or first pointer char
		rs[index] = tmp;
		//increment index to repeat process all the way to b
		index++;
	}
}
