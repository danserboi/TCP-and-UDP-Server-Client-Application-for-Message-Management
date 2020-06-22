// SERBOI FLOREA-DAN 325CB
#include "glist.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// inserare la inceput reusita sau nu (1/0)
int cons(void *element, list* l) {
	list temp = NULL;
	temp = calloc(1, sizeof(struct cell));
	if(!temp)
		return 0;
	temp->element = element;
	temp->next = *l;
	*l = temp;
	return 1;
}


// returneaza celula elementului care se doreste cautat
// NULL daca nu exista
list search(void *element, list l, int size) {
	list temp = NULL;
	while(l){
		temp = l->next;
		if(memcmp(element,l->element, size) == 0)
			return l;
		l = temp;
	}
	return NULL;
}

// returneaza 1/0 in caz de succes/esuare
int elim (void *element, list* l, int size) {
	list p = *l, ant = NULL, aux = NULL;
	while(p){
		if(memcmp(element,p->element, size) == 0){
			if(ant == NULL){
				*l = p->next;
			}
			else {
				ant->next = p->next;
			}
			aux = p;
			p = p->next;
			// intai eliberam spatiul alocat elementului
			free(aux->element);
			free(aux);
			return 1;
		}
		else {
			ant = p;
			p = p->next;
		}
	}
	return 0;
}

// elibereaza memoria alocata unei liste de elemente
void free_all(list l){
	list temp = NULL;
	while(l){
		temp = l->next;
		// intai eliberam spatiul alocat elementului
		free(l->element);
		free(l);
		l = temp;
	}
}