#ifndef GLOBAL_H
#define GLOBAL_H

struct response_data {
	char *memory;
	size_t size;
	size_t allocated_size;
};

struct thread_data {
    CURL *curl;
    struct curl_slist *nxt;
    int auth_type;
    struct response_data *r_data;
};

enum request_type {
    GET = 0,
    POST
};
#endif
