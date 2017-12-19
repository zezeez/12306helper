#ifndef COMMON_LIST_H
#define COMMON_LIST_H

struct common_list {
	void *data;
	struct common_list *next;
};

int init_list(struct common_list **);
int insert_node(struct common_list *, void *, void *(*func)(void *));
int remove_node(struct common_list *, void *, int (*func)(void *, void *));
struct common_list * find_node(struct common_list *, void *, int (*func)(void *, void *));
int clear_list(struct common_list *);

#endif
