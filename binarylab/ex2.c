//Implement a C function called ex2 such that it achieves 
//the same functionality as the machine code of objs/ex2_sol.o
//Note: you need to figure out ex2()'s function signature yourself; 
//the signature is not void ex2()
  
#include <assert.h>
#include <stdio.h>
//Function takes in a char pointer representing a char array(String)
//Traverses array and returns number of 'a' chars 
int
ex2(char* ptr) {
	//Initialize count of 'a' characters
	int count = 0;
	//Initialize temporary pointer to traverse argument
	char* tmp = ptr;
	//loop to traverse char array
	while (*tmp != '\0') {
		//printf("Char = %c\n", ptr[i]);
		//if 'a' is found, increment count
		if (*tmp == 'a') {
			count++;
		}
		//incrementing temporary pointer
		tmp++;
	}
	//printf("Count = %d\n", count);
	return count;
}
	