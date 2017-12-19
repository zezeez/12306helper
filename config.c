#include "config.h"

int load_config(struct user_config *uc)
{
    FILE *fd;
    char buffer[512];
    if((fd = fopen("./tickethelper.conf", "r")) == NULL) {
	perror("fopen: ");
	return -1;
    }
    while(fgets(buffer, sizeof(buffer), fd)) {
	parse_config(uc, buffer);
    }
    return 0;
}

int parse_config(struct user_config *uc, const char *buffer)
{
    const char *p = buffer;
    while(*p) {
	if(*p == ' ' || *p == '\t') {
	    continue;
	} else if(*p == '#') {
	    return 1;
	} else {
	}
	p++;
    }
    return 0;
}
