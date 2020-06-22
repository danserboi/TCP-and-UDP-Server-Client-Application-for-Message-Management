// SERBOI FLOREA-DAN 325CB
#ifndef LIST_H
#define LIST_H

typedef struct cell *list;

struct cell {
  void *element;
  list next;
};

extern int cons(void *element, list* l);
extern list search(void *element, list l, int size);
extern int elim (void *element, list* l, int size);
extern void free_all(list l);
extern void print_all_int (list l);

#endif