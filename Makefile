CFLAGS = -Wall -g
LFLAGS = -lcurl
DFLAGS = 
ticket: ordertickethelper.c utils.c cJSON.c varification.c config.c sendmail.c
	gcc `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` ordertickethelper.c varification.c cJSON.c utils.c config.c common_list.c sendmail.c -o ticket $(CFLAGS) $(LFLAGS) $(DFLAGS)
clean:
	rm ticket
