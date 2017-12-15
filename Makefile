ticket: ordertickethelper.c utils.c cJSON.c varification.c
	gcc `pkg-config --cflags gtk+-3.0` varification.c `pkg-config --libs gtk+-3.0` ordertickethelper.c cJSON.c utils.c -o ticket -lcurl
clean:
	rm ticket
