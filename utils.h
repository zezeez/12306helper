#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

struct station_name {
	char *simple_py;
	char *name;
	char *code;
	char *full_py;
	char *simple_py2;
	char *num;
};

struct common_list {
	void *data;
	struct common_list *next;
};

int init_list(struct common_list **);
int insert(struct common_list *, void *, size_t);
int remove_node(struct common_list *, void *, int (*func)(void *, void *));
struct common_list * find(struct common_list *, void *, int (*func)(void *, void *));
int clear_list(struct common_list *);

char ** split(const char *, const char);
int parse_peer_train(char *, void *);
void free_ptr_array(void **);
struct station_name * load_stations_name();
const char * find_station_name_by_code(struct station_name *, const char *, struct common_list *);
const char * find_station_name_at_cache(struct common_list *, const char *);
#endif
