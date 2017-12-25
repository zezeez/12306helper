#include "config.h"

int load_config(struct user_config *uc)
{
    FILE *fd;
    char buffer[512];
    char *p;
    char config_path[64];

    uc->_query_ticket_interval = 3000;
    uc->_block_time = 30;
    if(uc->_config_path[0] == 0) {
	strncpy(config_path, "./tickethelper.conf", sizeof(config_path));
    } else {
	strncpy(config_path, uc->_config_path, sizeof(config_path));
    }

    if((fd = fopen(config_path, "r")) == NULL) {
	perror("fopen: ");
	return -1;
    }
    while(fgets(buffer, sizeof(buffer), fd)) {
	p = strchr(buffer, '\n');
	if(p) {
	    *p = '\0';
	}
	parse_config(uc, buffer);
    }
    fclose(fd);
    return 0;
}

int set_time_level(struct user_config *uc, const char *value)
{
    char **f_time = split(value, ',');
    char **s_time;
    char **pf = f_time;
    int i = 0;
    if(f_time == NULL) {
	s_time = split(value, '-');
	if(s_time != NULL) {
	    if(s_time[0] && s_time[1]) {
		strncpy(uc->_t_level[0].time_start, s_time[0], sizeof(uc->_t_level[0].time_start));
		strncpy(uc->_t_level[0].time_end, s_time[1], sizeof(uc->_t_level[0].time_end));
	    }
	    free_ptr_array((void **)s_time);
	}
	return 0;
    }
    while(*pf) {
	s_time = split(*pf, '-');
	if(s_time != NULL) {
	    if(s_time[0] && s_time[1]) {
		strncpy(uc->_t_level[i].time_start, s_time[0], sizeof(uc->_t_level[i].time_start));
		strncpy(uc->_t_level[i].time_end, s_time[1], sizeof(uc->_t_level[i].time_end));
	    }
	    free_ptr_array((void **)s_time);
	}
	pf++;
	i++;
    }
    free_ptr_array((void **)f_time);
    return 0;
}

int set_train_info(char *config_value, size_t size, const char *value)
{
    char **pt = split(value, ',');
    char **pn = pt;
    int i = 0;
    if(pn == NULL) {
	strncpy(config_value, value, size);
	*(config_value + size)= '\0';
    } else {
	while(*pn) {
	    strncpy(config_value + i * size, *pn, size);
	    pn++;
	    i++;
	}
	*(config_value + i * size) = '\0';
	free_ptr_array((void **)pt);
    }
    return 0;
}

int set_config_value(struct user_config *uc, const char *key, const char *value)
{
    if(strcmp(key, "username") == 0) {
	strcpy(uc->_username, value);
    } else if(strcmp(key, "password") == 0) {
	strcpy(uc->_password, value);
    } else if(strcmp(key, "start_tour_date") == 0) {
	strcpy(uc->_start_tour_date, value);
    } else if(strcmp(key, "from_station_name") == 0) {
	strcpy(uc->_from_station_name, value);
    } else if(strcmp(key, "to_station_name") == 0) {
	strcpy(uc->_to_station_name, value);
    } else if(strcmp(key, "query_ticket_interval") == 0) {
	uc->_query_ticket_interval = (int)strtol(value, NULL, 10);
    } else if(strcmp(key, "max_queue_count") == 0) {
	uc->_max_queue_count = (int)strtol(value, NULL, 10);
    } else if(strcmp(key, "block_time") == 0) {
	uc->_block_time = (int)strtol(value, NULL, 10);
    } else if(strcmp(key, "prefer_train_type") == 0) {
	//strcpy(uc->_prefer_train_type, value);
	set_train_info(*uc->_prefer_train_type, sizeof(*uc->_prefer_train_type), value);
    } else if(strcmp(key, "prefer_train_no") == 0) {
	//strcpy(uc->_prefer_train_no, value);
	set_train_info(*uc->_prefer_train_no, sizeof(*uc->_prefer_train_no), value);
    } else if(strcmp(key, "prefer_seat_type") == 0) {
	strcpy(uc->_prefer_seat_type_all, value);
	set_train_info(*uc->_prefer_seat_type, sizeof(*uc->_prefer_seat_type), value);
    } else if(strcmp(key, "prefer_ticket_time") == 0) {
	set_time_level(uc, value);
    } else if(strcmp(key, "use_cdn_server_file") == 0) {
	strcpy(uc->_use_cdn_server_file, value);
    } else if(strcmp(key, "passenger_name") == 0) {
	strcpy(uc->_passenger_name, value);
    } else if(strcmp(key, "mail_username") == 0) {
	strcpy(uc->_mail_username, value);
    } else if(strcmp(key, "mail_password") == 0) {
	strcpy(uc->_mail_password, value);
    } else if(strcmp(key, "mail_server") == 0) {
	strcpy(uc->_mail_server, value);
    }
    return 0;
}

int parse_config(struct user_config *uc, const char *buffer)
{
    const char *p = buffer;
    char key[64], value[64];
    char **split_ptr;
    while(*p) {
	if(*p ==  ' ' || *p == '\t') {
	    p++;
	} else if(*p == '#') {
	    return 1;
	} else {
	    break;
	}
    }
    split_ptr = split(p, '=');
    if(split_ptr[0]) {
	trim_space(split_ptr[0], key);
    }
    if(split_ptr[1]) {
	trim_space(split_ptr[1], value);
    }
    free_ptr_array((void **)split_ptr);
    set_config_value(uc, key, value);
    return 0;
}

void print_config(struct user_config *uc)
{
    printf("username: %s.\npassword: %s.\nstart_tour_date: %s.\nfrom_station_name: %s.\nto_station_name: %s.\nquery_ticket_interval: %d.\nmax_queue_count: %d.\nuse_cdn_server_file: %s.\npassenger_name: %s.\nmail_username: %s.\nmail_password: %s.\nmail_server: %s.\n", uc->_username, uc->_password, uc->_start_tour_date,
	    uc->_from_station_name, uc->_to_station_name, uc->_query_ticket_interval, 
	    uc->_max_queue_count,
	    uc->_use_cdn_server_file, uc->_passenger_name,
	    uc->_mail_username, uc->_mail_password, uc->_mail_server);
    int i = 0;
    printf("prefer_train_type: ");
    while(uc->_prefer_train_type[i][0]) {
	printf("%s,", uc->_prefer_train_type[i]);
	i++;
    }
    putchar('\n');
    i = 0;
    printf("prefer_train_no: ");
    while(uc->_prefer_train_no[i][0]) {
	printf("%s,", uc->_prefer_train_no[i]);
	i++;
    }
    putchar('\n');
    i = 0;
    printf("prefer_seat_type: ");
    while(uc->_prefer_seat_type[i][0]) {
	printf("%s,", uc->_prefer_seat_type[i]);
	i++;
    }
    putchar('\n');
    i = 0;
    while(uc->_t_level[i].time_start[0]) {
	printf("time_start: %s, time_end: %s\n", uc->_t_level[i].time_start, uc->_t_level[i].time_end);
	i++;
    }
}

