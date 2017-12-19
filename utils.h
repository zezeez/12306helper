#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "common_list.h"

struct station_name {
	char *simple_py;
	char *name;
	char *code;
	char *full_py;
	char *simple_py2;
	char *num;
};

char ** split(const char *, const char);
int parse_peer_train(char *, void *);
void free_ptr_array(void **);
int load_stations_name(struct common_list *);
//const char * find_station_name_by_code(struct station_name *, const char *, struct common_list *);
//const char * find_station_name_at_cache(struct common_list *, const char *);
int find_station_name(void *, void *);
int find_station_code(void *, void *);
void *insert_station_name(void *);
int remove_station_name(void *);
int trim_space(const char *, char *);
#endif
