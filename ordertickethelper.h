#ifndef ORDERTICKETHELPER_H
#define ORDERTICKETHELPER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <curl/curl.h>
#include <ncurses.h>
#include "cJSON.h"
#include "utils.h"
#include "global.h"
#include "config.h"

#define USE_COOKIE 1
#define TARGETDOMAIN "https://kyfw.12306.cn"
#define BASEURL "https://kyfw.12306.cn/otn/"

struct passenger_info {
    char code[4];
    char passenger_name[16];
    char sex_code[4];
    char sex_name[4];
    char born_date[24];
    char address[64];
    char country_code[4];
    char email[64];
    char first_letter[8];
    char index_id[4];
    char mobile_no[16];
    char passenger_flag[4];
    char passenger_id_no[32];
    char passenger_id_type_code[4];
    char passenger_id_type_name[32];
    char passenger_type[4];
    char passenger_type_name[8];
    char phone_no[16];
    char postalcode[16];
    char record_count[4];
    char total_times[8];
};

struct train_info {
    char *secretStr;
    char *textInfo;
    char *train_no;
    char *station_train_code;
    char *start_station_telecode;
    char *end_station_telecode;
    char *from_station_telecode;
    char *to_station_telecode;
    char *start_time;
    char *arrive_time;
    char *spend_time;
    char *can_web_buy;
    char *yp_info;
    char *start_train_date;
    char *train_seat_feature;
    char *location_code;
    char *from_station_no;
    char *to_station_no;
    char *is_support_card;
    char *controlled_train_flag;
    char *gg_num;
    char *gr_num;
    char *qt_num;
    char *rw_num;
    char *rz_num;
    char *tz_num;
    char *wz_num;
    char *yb_num;
    char *yw_num;
    char *yz_num;
    char *ze_num;
    char *zy_num;
    char *swz_num;
    char *srrb_num;
    char *yp_ex;
    char *seat_types;
    char *from_station_name;
    char *to_station_name;
};

struct ticket_info {
    char repeat_submit_token[64];
    //char init_seatTypes[256];
    //char defaultTicketTypes[256];
    //char init_cardTypes[256];
    //char ticket_seat_codeMap[1024];
    char ticketInfoForPassengerForm[8192];
    //char orderRequestDTO[512];
    char left_ticket[64];
    char key_is_change[64];
    char seat_type[4];
    char train_location[4];
};

struct screen_param {
    WINDOW *status;
    WINDOW *output;
    WINDOW *info;
    int rows;
    int cols;
};

int parse_train_info(const char *, char (*)[512], struct train_info *, struct common_list *);
int perform_request(const char *, enum request_type, void *, struct curl_slist *);
size_t write_memory_callback(void *, size_t, size_t, void *);
void print_train_info(struct train_info *);
int init_buffer();
int init_curl();
int do_cleanup();
int clean_curl();
int query_ticket();
int show_varification_code();
int start_submit_order_request(struct train_info *);
int get_passenger_tickets_for_auto_submit();
int checkUserIsLogin();
int get_passenger_dtos(const char *);
int passenger_initdc();
int submit_order_request(struct train_info *);
int init_my12306();
int user_login();
int init_user_screen();
extern void *show_varification_code_main(void *);
static void sig_handler(int);
#endif
