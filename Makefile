CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -g
LFLAGS = `pkg-config --libs gtk+-3.0` -lcurl
DFLAGS = -DUSE_COOKIE
BUILD_OBJS = ordertickethelper.c \
	     utils.c \
	     cJSON.c \
	     varification.c \
	     config.c \
	     sendmail.c \
	     common_list.c

tickethelper: $(BUILD_OBJS)
	gcc $(CFLAGS) $(DFLAGS) $(BUILD_OBJS) -o $@ $(LFLAGS)
clean:
	rm tickethelper
