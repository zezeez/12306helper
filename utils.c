#include "utils.h"

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

int insert(struct common_list *l, void *d, size_t size)
{
	struct common_list *node = (struct common_list *) malloc(sizeof(struct common_list));
	if(node == NULL) {
		perror("malloc: ");
		return -1;
	}
	void *ptr = malloc(size);
	if(ptr == NULL) {
		perror("malloc: ");
		return -1;
	}
	memcpy(ptr, d, size);
	node->data = ptr;
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
			break;
		}
	}
	if(p) {
		prev->next = p->next;
		free(p->data);
		free(p);
		return 0;
	} else {
		return -1;
	}
}

struct common_list * find(struct common_list *l, void *d, int (*func)(void *, void *))
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

int clear_list(struct common_list *l)
{
	struct common_list *p = l;
	struct common_list *tmp = p;
	while(tmp) {
		tmp = p->next;
		free(p->data);
		free(p);
		p = tmp;
	}
	return 0;
}

char ** split(const char *s, const char c)
{
    const char *p = s;
    const char *step = p;
    const char *s_pos, *e_pos;
    char **target_str_array = NULL;
    int sep_count = 0;
    int array_size = 64;

    target_str_array = (char **) malloc(array_size * sizeof(char *));
    if(target_str_array == NULL) {
	perror("malloc: ");
	return NULL;
    }

    while(*p) {
	s_pos = step;
	while(*step != c && *step) {
	    step++;
	}
	e_pos = step;
	if(*step) {
	    step++;  /* skip separator */
	}
	if(*step || (!*step && s_pos != s)) {
	    if(sep_count >= array_size - 1) {
		target_str_array = (char **) realloc(target_str_array, array_size * sizeof(char *) * 2);
		if(target_str_array == NULL) {
		    perror("realloc: ");
		    return NULL;
		}
		array_size += array_size;
	    }
	    target_str_array[sep_count] = (char *) malloc((e_pos - s_pos + 1) * sizeof(char));
	    if(target_str_array[sep_count] == NULL) {
		perror("malloc: ");
		int i = 0;
		while(i < sep_count) {
		    free(target_str_array[i]);
		    i++;
		}
		free(target_str_array);
		return NULL;
	    }
	    memcpy(target_str_array[sep_count], s_pos, e_pos - s_pos);
	    target_str_array[sep_count][e_pos - s_pos] = '\0';
	    sep_count++;
	}
	p = step;
    }
    if(sep_count) {
	target_str_array[sep_count] = NULL;
	return target_str_array;
    } else {
	//free(target_str_array);
	return NULL;
    }
}

void free_ptr_array(void **ptr)
{
	void **p = ptr;
	while(*p) {
		free(*p);
		(void)p++;
	}
	free(ptr);
}

int parse_peer_train(char *buff, void *ti)
{
    char *pb = buff;
    void **ptr = ti;
    *ptr = pb;
    ptr++;
    while(*pb) {
	if(*pb == '|') {
	    *pb = '\0';
	    *ptr = pb + 1;
	    ptr++;
	}
	pb++;
    }
    return 0;
}

struct station_name * load_stations_name()
{
    FILE *fd;
    char buffer[102400];
    char **s;
    struct station_name *s_name;
    int size = 64;
    s_name = (struct station_name *) malloc(size * sizeof(struct station_name));
    if(s_name == NULL) {
	perror("malloc: ");
	return NULL;
    }
    if((fd = fopen("./station_name.txt", "r")) == NULL) {
	perror("fopen: ");
	return NULL;
    }
    if(fgets(buffer, sizeof(buffer), fd) == NULL) {
	perror("fgets: ");
	fclose(fd);
	return NULL;
    }
    fclose(fd);
    s = split(buffer, '@');
    if(s) {
	int i = 0;
	char **station;
	while(s[i]) {
	    station = split(s[i], '|');
	    if(i >= size - 1) {
		s_name = (struct station_name *) realloc(s_name, size * sizeof(struct station_name) * 2);
		if(s_name == NULL) {
		    perror("realloc: ");
		    free_ptr_array((void **)s);
		    return NULL;
		}
		size += size;
	    }
	    memcpy(s_name + i, station, sizeof(struct station_name));
	    free(station);
	    i++;
	}
	memset(s_name + i, 0, sizeof(struct station_name));
	return s_name;
    } else {
	return NULL;
    }
}

const char * find_station_name_by_code(struct station_name *name, const char *code, struct common_list *cache)
{
    struct station_name *pn = name;
    while(pn->simple_py) {
	if(strcmp(pn->code, code) == 0) {
	    insert(cache, pn, sizeof(struct station_name));
	    return pn->name;
	}
	pn++;
    }
    return NULL;  /* not found */
}

const char * find_station_name_at_cache(struct common_list *cache, const char *code)
{
    struct common_list *p = cache;
    while(p) {
	if(strcmp(((struct station_name *)p->data)->code, code)) {
	    p = p->next;
	} else {
	    return ((struct station_name *)p->data)->name;
	}
    }
    return NULL;  /* not found */
}

