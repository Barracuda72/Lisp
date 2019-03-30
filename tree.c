#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"

tree *tree_create_ex(tree *left, tree *right, int type, int id, void *param)
{
  tree *r = malloc(sizeof(tree));
  memset(r, 0, sizeof(tree));

  r->left = left;
  r->right = right;
  r->type = type;
  r->id = id;
  r->param = param;

  return r;
}

tree *tree_create(tree *left, tree *right, int type)
{
  return tree_create_ex(left, right, type, -1, NULL);
}

tree *tree_link_lists(tree *head, tree *tail)
{
  if (head == NULL)
  {
    return tail;
  } else {
    tree *h = head;

    while (head->next != NULL)
      head = head->next;

    head->next = tail;

    return h;
  }
}
