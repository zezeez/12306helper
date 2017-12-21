#include "utils.h"

char **split(const char *s, const char c)
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

int load_stations_name(struct common_list *cl)
{
    FILE *fd;
    char buffer[102400];
    char **s;
    /*struct station_name *s_name;
    int size = 64;
    s_name = (struct station_name *) malloc(size * sizeof(struct station_name));
    if(s_name == NULL) {
	perror("malloc: ");
	return NULL;
    }*/
    if((fd = fopen("./station_name.txt", "r")) == NULL) {
	perror("fopen: ");
	return -1;
    }
    if(fgets(buffer, sizeof(buffer), fd) == NULL) {
	perror("fgets: ");
	fclose(fd);
	return -1;
    }
    fclose(fd);
    s = split(buffer, '@');
    if(s) {
	int i = 0;
	char **station;
	while(s[i]) {
	    station = split(s[i], '|');
	    /*if(i >= size - 1) {
		s_name = (struct station_name *) realloc(s_name, size * sizeof(struct station_name) * 2);
		if(s_name == NULL) {
		    perror("realloc: ");
		    free_ptr_array((void **)s);
		    return NULL;
		}
		size += size;
	    }
	    memcpy(s_name + i, station, sizeof(struct station_name));
	    */
	    insert_node(cl, (struct station_name *) station, insert_station_name);
	    free(station);
	    i++;
	}
	//memset(s_name + i, 0, sizeof(struct station_name));
	return 0;
    } else {
	return 1;
    }
}

void *insert_station_name(void *s)
{
    struct station_name *p = (struct station_name *) malloc(sizeof(struct station_name));
    if(p == NULL) {
	perror("malloc: ");
	return NULL;
    }
    memcpy(p, s, sizeof(struct station_name));
    return p;
}

int find_station_name(void *s, void *d)
{
    struct station_name *s_name = (struct station_name *) s;
    //struct station_name *d_name = (struct station_name *) d;
    if(strcmp(s_name->code, (const char *)d) == 0) {
	return 0;
    }
    return 1;
}

int find_station_code(void *s, void *d)
{
    struct station_name *s_name = (struct station_name *) s;
    //struct station_name *d_name = (struct station_name *) d;
    if(strcmp(s_name->name, (const char *)d) == 0) {
	return 0;
    }
    return 1;
}

int remove_station_name(void *d)
{
    struct station_name *p = (struct station_name *) d;
    free(p->simple_py);
    free(p->name);
    free(p->code);
    free(p->full_py);
    free(p->simple_py2);
    free(p->num);
    return 0;
}

/*const char * find_station_name_by_code(struct station_name *name, const char *code, struct common_list *cache)
{
    struct station_name *pn = name;
    while(pn->simple_py) {
	if(strcmp(pn->code, code) == 0) {
	    insert(cache, pn, sizeof(struct station_name));
	    return pn->name;
	}
	pn++;
    }
    return NULL; 
}*/

/*const char * find_station_name_at_cache(struct common_list *cache, const char *code)
{
    struct common_list *p = cache;
    while(p) {
	if(strcmp(((struct station_name *)p->data)->code, code)) {
	    p = p->next;
	} else {
	    return ((struct station_name *)p->data)->name;
	}
    }
    return NULL; 
}*/

int trim_space(const char *str, char *buffer)
{
    const char *s_ptr = str, *e_ptr;
    s_ptr = str;
    while(*s_ptr) {
	if(*s_ptr == ' ' || *s_ptr == '\t') {
	    s_ptr++;
	} else {
	    break;
	}
    }
    e_ptr = s_ptr;
    while(*e_ptr) {
	if(*e_ptr != ' ' && *e_ptr != '\t') {
	    e_ptr++;
	} else {
	    break;
	}
    }
    memcpy(buffer, s_ptr, e_ptr - s_ptr);
    buffer[e_ptr - s_ptr] = '\0';
    return 0;
}

void *read_file_all(const char *file)
{
    FILE *fd;
    size_t fsize;
    char *buffer;

    if((fd = fopen(file, "rb")) == NULL) {
	perror("fopen: ");
	return NULL;
    }

    fseek(fd, 0, SEEK_END);
    fsize = ftell(fd);
    rewind(fd);

    buffer = (char *) malloc(sizeof(char) * fsize);
    if(buffer == NULL) {
	perror("malloc: ");
	fclose(fd);
	return NULL;
    }
    if(fread(buffer, 1, fsize, fd) != fsize && !feof(fd)) {
	perror("perror: ");
	fclose(fd);
	free(buffer);
	return NULL;
    }

    fclose(fd);
    return buffer;
}
