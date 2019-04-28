#pragma once

typedef struct tnode {
	char *string;
	int count;
	struct tnode *left;
	struct tnode *right;
} tnode_t;


