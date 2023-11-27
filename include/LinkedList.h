#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>

typedef struct Node {
  int val;
  struct Node *next;
} Node;

Node *newList(int val);

Node *addNext(Node *node, int val);

#endif