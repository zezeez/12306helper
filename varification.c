#include <gtk/gtk.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cJSON.h"
#include "global.h"

struct coordinate {
	gint x;
	gint y;
};

struct thread_res {
    gboolean is_selected[8];
    struct coordinate cor[8];
    GtkWidget *selected_image_widget[8];
    GtkWidget *image_widget;
    GtkWidget *layout;
    GtkWidget *window;
    struct thread_data *tdata;
};

static void refresh(GtkWidget *, gpointer);
extern int perform_request(const char *, enum request_type, void *, struct curl_slist *);
extern size_t write_memory_callback(void *, size_t, size_t, void *);

void on_selected(struct thread_res *, gint, gint, gint);
void on_unselected(struct thread_res *, gint);
int get_current_position();
void clear_selection(struct thread_res *);

static gboolean success = FALSE;

static gboolean
button_press_event_cb (GtkWidget      *widget,
                       GdkEventButton *event,
                       gpointer        data)
{
    struct thread_res *tres = (struct thread_res *) data;
    if (event->button == GDK_BUTTON_PRIMARY)
    {
	g_print("button_primary\n");
	g_print("x: %d, y: %d\n", (gint)event->x, (gint)event->y);
	//gtk_widget_queue_draw_area(image_widget, event->x, event->y, 32, 32);
	int pos = get_current_position((gint)event->x, (gint)event->y);
	if(pos != -1) {
	    if(!tres->is_selected[pos]) {
		on_selected(tres, pos, (gint)event->x, (gint)event->y);
		tres->cor[pos].x = (int) event->x;
		tres->cor[pos].y = (int) event->y - 30;
	    } else {
		on_unselected(tres, pos);
		tres->cor[pos].x = 0;
		tres->cor[pos].y = 0;
	    }
	    tres->is_selected[pos] = ~tres->is_selected[pos];
	}
	/*for(i = 0; i < 8; i++) {
	  if(cor[i].x == 0 && cor[i].y == 0) {
	  cor[i].x = (int) event->x;
	  cor[i].y = (int) event->y - 30;
	  break;
	  }
	  }*/
    }
    else if (event->button == GDK_BUTTON_SECONDARY)
    {
	g_print("button_secondary\n");
	g_print("x: %d, y: %d\n", (gint)event->x, (gint)event->y);
	clear_selection(tres);
    }

    /* We've handled the event, stop processing */
    return TRUE;
}

int get_current_position(gint x, gint y)
{
	if(x < 72 && y >= 40 && y < 110) {
		return 0;
	} else if(x >= 72 && x < 144 && y >= 40 && y < 110) {
		return 1;
	} else if(x >= 144 && x < 216 && y >= 40 && y < 110) {
		return 2;
	} else if(x >= 216 && x < 288 && y >= 40 && y < 110) {
		return 3;
	} else if(x < 72 && y >= 110 && y < 180) {
		return 4;
	} else if(x >= 72 && x < 144 && y >= 110 && y < 180) {
		return 5;
	} else if(x >= 144 && x < 216 && y >= 110 && y < 180) {
		return 6;
	} else if(x >= 216 && x < 288 && y >= 110 && y < 180) {
		return 7;
	}
	return -1;
}

void on_selected(struct thread_res *t, gint i, gint x, gint y)
{
    if(t->selected_image_widget[i] == NULL) {
	t->selected_image_widget[i] = gtk_image_new_from_file("./images/img_selected.png");
	if(t->selected_image_widget[i]) {
	    gtk_layout_put(GTK_LAYOUT(t->layout), t->selected_image_widget[i], x - 16, y - 16);
	    gtk_widget_show(t->selected_image_widget[i]);
	}
    } else {
	gtk_layout_move(GTK_LAYOUT(t->layout), t->selected_image_widget[i], x - 16, y - 16);
	gtk_widget_show(t->selected_image_widget[i]);
    }
}

void on_unselected(struct thread_res *t, gint i)
{
    if(t->selected_image_widget[i]) {
	gtk_widget_hide(t->selected_image_widget[i]);
    }
}

void clear_selection(struct thread_res *t)
{
    int i;
    for(i = 0; i < 8; i++) {
	if(t->is_selected[i]) {
	    t->is_selected[i] = FALSE;
	}
	if(t->cor[i].x != 0 && t->cor[i].y != 0) {
	    t->cor[i].x = 0;
	    t->cor[i].y = 0;
	}
	if(t->selected_image_widget[i] != NULL) {
	    if(gtk_widget_get_visible(t->selected_image_widget[i])) {
		gtk_widget_hide(t->selected_image_widget[i]);
	    }
	}
    }
}


static int 
do_varification(void *user_data)
{
    char url[64];
    char str_cor[64];
    int i;
    char *p_str_cor = str_cor;
    struct thread_res *tres = (struct thread_res *) user_data;

    snprintf(url, sizeof(url), "%s", "https://kyfw.12306.cn/passport/captcha/captcha-check");
    memset(str_cor, 0, sizeof(str_cor));

    for(i = 0; i < 8; i++) {
	if(tres->is_selected[i]) {
	    snprintf(p_str_cor, sizeof(str_cor), "%d,%d,", tres->cor[i].x, tres->cor[i].y);
	    p_str_cor += strlen(p_str_cor);
	    //tres->cor[i].x = 0;
	    //tres->cor[i].y = 0;
	    //tres->is_selected[i] = FALSE;
	}
    }
    if(i) {
	str_cor[strlen(str_cor) - 1] = '\0';
    }
    char *urlencode = curl_easy_escape(tres->tdata->curl, str_cor, strlen(str_cor));
    printf("str_cor: %s\n", str_cor);
    printf("urlencode: %s\n", urlencode);
    char post_data[64];

    if(tres->tdata->auth_type == 0) {
	snprintf(post_data, sizeof(post_data), "answer=%s&login_site=E&rand=sjrand", urlencode);
    } else {
	snprintf(post_data, sizeof(post_data), "answer=%s&login_site=E&rand=randp", urlencode);
    }

    if(perform_request(url, POST, post_data, tres->tdata->nxt) < 0) {
	return -1;
    }
    cJSON *ret_json = cJSON_Parse(tres->tdata->r_data->memory);
    if(!ret_json) {
	fprintf(stderr, "cJSON_Parse: %s\n", cJSON_GetErrorPtr());
	return -1;
    }
    cJSON *ret_code = cJSON_GetObjectItem(ret_json, "result_code");
    if(!cJSON_IsString(ret_code)) {
	fprintf(stderr, "error parse json data\n");
	return -1;
    }
    if(strcmp(ret_code->valuestring, "4") == 0) {
	cJSON_Delete(ret_json);
	curl_free(urlencode);
	return 0;
    } else {
	printf("%s\n", tres->tdata->r_data->memory);
	refresh(tres->image_widget, user_data);
    }
    return 1;
}

static void
submit(GtkWidget *widget,
	gpointer user_data)
{
    struct thread_res *tres = (struct thread_res *) user_data;
    if(do_varification(user_data) == 0) {
	//gtk_main_quit();
	//g_object_unref(tres->window);
	//gtk_widget_destroy(tres->window);
	//g_object_unref(g_app);
	success = TRUE;
	g_signal_emit_by_name(tres->window, "destroy");
	//g_application_quit(G_APPLICATION(tres->g_app));
	//gtk_widget_destroy(tres->window);
	//pthread_exit((void *)ret);
    }
}

static void
refresh(GtkWidget *widget,
	gpointer user_data)
{
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;
    GError *error = NULL;
    gboolean ret;
    char url[256];
    double num;

    struct thread_res *tres = (struct thread_res *) user_data;
    clear_selection(user_data);

    num = rand() / RAND_MAX;
    if(tres->tdata->auth_type == 0) {
	snprintf(url, sizeof(url), "%s%.16lf", "https://kyfw.12306.cn/passport/captcha/captcha-image?login_site=E&module=login&rand=sjrand&", num);
    } else {
	snprintf(url, sizeof(url), "%s%.16lf", "https://kyfw.12306.cn/passport/captcha/captcha-image?login_site=E&module=passenger&rand=randp&", num);
    }


    if(perform_request(url, GET, NULL, tres->tdata->nxt) < 0) {
	return;
    }
    static int counter = 0;
    loader = gdk_pixbuf_loader_new();
    ret = gdk_pixbuf_loader_write(loader, (const guchar *)tres->tdata->r_data->memory, tres->tdata->r_data->size, &error);
    g_print("ret: %d, counter: %d\n", ret, ++counter);
    if(ret) {
	pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
	if(pixbuf) {
	    gtk_image_set_from_pixbuf(GTK_IMAGE(tres->image_widget), pixbuf);
	    g_object_unref(pixbuf);
	}
	//g_object_unref(loader);
    }
    error = NULL;
    gdk_pixbuf_loader_close(loader, &error);
    //g_object_unref(error);
}
 
//void on_active(GtkWidget * widget, GdkEvent *event, gpointer data)
void on_active(GtkWidget *window, void *user_data)
{
    GtkWidget *event_box;
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;
    GError *error = NULL;
    gboolean ret;
    double num;
    char url[128];
    struct thread_res *tres = (struct thread_res *) user_data;

    num = (double)rand() / RAND_MAX;

    if(tres->tdata->auth_type == 0) {
	snprintf(url, sizeof(url), "%s%.16lf", "https://kyfw.12306.cn/passport/captcha/captcha-image?login_site=E&module=login&rand=sjrand&", num);
    } else {
	snprintf(url, sizeof(url), "%s%.16lf", "https://kyfw.12306.cn/passport/captcha/captcha-image?login_site=E&module=passenger&rand=randp&", num);
    }

    if(perform_request(url, GET, NULL, tres->tdata->nxt)) {
	return;
    }

    loader = gdk_pixbuf_loader_new();
    ret = gdk_pixbuf_loader_write(loader, (const guchar *)tres->tdata->r_data->memory, tres->tdata->r_data->size, &error);
    if(ret) {
	    pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
	    tres->image_widget = gtk_image_new_from_pixbuf(pixbuf);
	    if(pixbuf) {
		g_object_unref(pixbuf);
	    }
	    //g_object_unref(loader);
	    event_box = gtk_event_box_new();
	    gtk_container_add (GTK_CONTAINER (window), event_box);
	    gtk_container_add (GTK_CONTAINER (event_box), tres->image_widget);

	    //g_signal_connect (event_box, "draw", G_CALLBACK (on_expose_event), NULL);
	    g_signal_connect (event_box, "button-press-event",
			    G_CALLBACK (button_press_event_cb), user_data);
	    gtk_widget_set_events (event_box, gtk_widget_get_events (tres->image_widget)
			    | GDK_BUTTON_PRESS_MASK/* | GDK_EXPOSURE_MASK*/);
	    //gtk_widget_set_app_paintable(image_widget, TRUE);
    }
    gdk_pixbuf_loader_close(loader, &error);
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *button_box;
    GtkWidget *box;
    static struct thread_res tres;

    memset(&tres, 0, sizeof(tres));
    tres.tdata = (struct thread_data *) user_data;

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "验证码");
    gtk_window_set_default_size (GTK_WINDOW (window), 295, 250);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
    tres.window = window;

    tres.layout = gtk_layout_new(NULL, NULL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);

    on_active(box, (void *)&tres);
    //gtk_container_add (GTK_CONTAINER (window), box);
    //g_signal_connect (GTK_WINDOW(window), "expose-event", G_CALLBACK (on_expose_event), NULL);

    button = gtk_button_new_with_label ("刷新");
    g_signal_connect (button, "clicked", G_CALLBACK (refresh), (void *)&tres);
    //g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
  
    gtk_container_add (GTK_CONTAINER (button_box), button);
    button = gtk_button_new_with_label ("提交");
    g_signal_connect (button, "clicked", G_CALLBACK (submit), (void *)&tres);
    gtk_container_add (GTK_CONTAINER (button_box), button);
    gtk_container_add (GTK_CONTAINER (box), button_box);
    gtk_layout_put(GTK_LAYOUT(tres.layout), box, 0, 0);
    gtk_container_add(GTK_CONTAINER(window), tres.layout);

    gtk_widget_show_all (window);
}

void *
show_varification_code_main (void *data)
{
    GtkApplication *app;
    char *argv[1];
    argv[0] = "a.out";
    int *ret_val = (int *) malloc(sizeof(int));

    app = gtk_application_new ("org.gtk.ticket", G_APPLICATION_FLAGS_NONE);

    g_signal_connect (app, "activate", G_CALLBACK (activate), data);
    *ret_val = g_application_run (G_APPLICATION (app), 1, argv);
    g_object_unref (app);
    if(success) {
	*ret_val = 100;  /* we return 100 when varification success */
    }

    return (void *)ret_val;
}
