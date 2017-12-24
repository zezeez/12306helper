#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <curl/curl.h>
#include "common_list.h"

struct station_name {
	char *simple_py;
	char *name;
	char *code;
	char *full_py;
	char *simple_py2;
	char *num;
};

struct train_black_list {
    char *train_no;
    time_t block_time_end;
};

char **split(const char *, const char);
int parse_peer_train(char *, void *);
void free_ptr_array(void **);
int load_stations_name(struct common_list *);
int find_station_name(void *, void *);
int find_station_code(void *, void *);
void *insert_station_name(void *);
int remove_station_name(void *);
void *insert_black_list(void *);
int remove_black_list(void *);
int find_black_list(void *, void *);
int find_and_remove_black_list(void *, void *);
int trim_space(const char *, char *);
void *read_file_all(const char *);
int load_cdn_server(struct curl_slist **, const char *);
#endif
