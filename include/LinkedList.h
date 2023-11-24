#include <stdlib.h>

typedef struct Node {
  int val;
  struct Node *next;
} Node;

Node *newList(int val);

Node *addNext(Node *node, int val);