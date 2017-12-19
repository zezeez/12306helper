#include "common_list.h"

int init_list(struct common_list **l)
{
    (*l) = (struct common_list *) malloc(sizeof(struct common_list));
    if(*l == NULL) {
	perror("malloc: ");
	return -1;
    }
    (*l)->data = NULL;
    (*l)->next = NULL;
    return 0;
}

int insert_node(struct common_list *l, void *d, void *(*func)(void *))
{
    struct common_list *node = (struct common_list *) malloc(sizeof(struct common_list));
    if(node == NULL) {
	perror("malloc: ");
	return -1;
    }
    /*void *ptr = malloc(size);
      if(ptr == NULL) {
      perror("malloc: ");
      return -1;
      }
      memcpy(ptr, d, size);*/
    node->data = func(d);
    node->next = l->next;
    l->next = node;
    return 0;
}

int remove_node(struct common_list *l, void *d, int (*func)(void *, void *))
{
    struct common_list *p = l->next;
    struct common_list *prev = l;
    while(p) {
	if(func(p->data, d) != 0) {
	    prev = p;
	    p = p->next;
	} else {
	    prev->next = p->next;
	    free(p);
	    return 0;
	}
    }
    return 1;
}

struct common_list *find_node(struct common_list *l, void *d, int (*func)(void *, void *))
{
    struct common_list *p = l->next;
    while(p) {
	if(func(p->data, d) != 0) {
	    p = p->next;
	} else {
	    return p;
	}
    }
    return NULL;  /* not found */
}

int clear_list(struct common_list *l, int (*func)(void *))
{
    struct common_list *p = l;
    struct common_list *tmp = p;
    while(tmp) {
	tmp = p->next;
	//free(p->data);
	//free(p);
	func(p->data);
	free(p);
	p = tmp;
    }
    return 0;
}
