#include "config.h"

int load_config(struct user_config *uc)
{
    FILE *fd;
    char buffer[512];
    char *p;
    if((fd = fopen("./tickethelper.conf", "r")) == NULL) {
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

int string_time_to_number(const char *t)
{

}

int set_time_level(struct user_config *uc, const char *value)
{
    char **f_time = split(value, ',');
    char **s_time;
    char **pf = f_time;
    char **ps;
    if(f_time == NULL) {
    }
    while(*p) {
	s_time = split(*p, '-');
	ps = s_time;

	p++;
    }
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
    } else if(strcmp(key, "aways_queue") == 0) {
	uc->_aways_queue = (int)strtol(value, NULL, 10);
    } else if(strcmp(key, "prefer_train_type") == 0) {
	strcpy(uc->_prefer_train_type, value);
    } else if(strcmp(key, "prefer_train_no") == 0) {
	strcpy(uc->_prefer_train_no, value);
    } else if(strcmp(key, "prefer_seat_type") == 0) {
	strcpy(uc->_prefer_seat_type, value);
    } else if(strcmp(key, "prefer_ticket_time") == 0) {
	strcpy(uc->_prefer_ticket_time, value);
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
    } else if(strcmp(key, "prefer_ticket_time") == 0) {
	char **sp_time = split(value, ',');
	if(sp_time == NULL) {
	}
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
    printf("username: %s.\npassword: %s.\nstart_tour_date: %s.\nfrom_station_name: %s.\nto_station_name: %s.\nquery_ticket_interval: %d.\naways_queue: %d.\nprefer_train_type: %s.\nrefer_seat_type: %s.\nprefer_ticket_time: %s.\nuse_cdn_server_file: %s.\npassenger_name: %s.\nmail_username: %s.\nmail_password: %s.\nmail_server: %s.\n", uc->_username, uc->_password, uc->_start_tour_date,
	    uc->_from_station_name, uc->_to_station_name, uc->_query_ticket_interval, 
	    uc->_aways_queue, uc->_prefer_train_type, uc->_prefer_seat_type, 
	    uc->_prefer_ticket_time, uc->_use_cdn_server_file, uc->_passenger_name,
	    uc->_mail_username, uc->_mail_password, uc->_mail_server);
}

