//Implement a C function called ex5 such that it achieves 
//the same functionality as the machine code of objs/ex5_sol.o
//Note: you need to figure out ex5()'s function signature yourself; 
//the signature is not void ex5(node *n). However, as a hint, 
//ex5 does take node * as an argument.
 
#include <assert.h>
#include "ex5.h"
#include <stdlib.h>
#include <stddef.h>

int
ex5(struct node *head)
{	
	//Check to see if head is Null
	if (head == NULL) {
		return 1;
	}
	//create a temporary node equal to head
	struct node *temp = head;

	//Loop to iterate through every node
	while (temp->next != NULL) {
		//check to ensure the next element value is greater than temp value
		if (temp->next->value < temp->value) {
			//break and return 0 if check comes back trues
			return 0;
		}
		//increment temp node
		temp = temp->next;
	}
	//Return 1 if loop gets through entire lisetlist
	return 1;
	
}

