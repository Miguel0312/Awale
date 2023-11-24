#include "../include/LinkedList.h"

Node* newList(int val) {
  Node* root = malloc(sizeof(Node));
  root->val = val;
  root->next = NULL;
  return root;
}

Node* addNext(Node* node, int val) {
  Node* newNode = newList(val);
  node->next = newNode;
  return newNode;
}