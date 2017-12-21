#include "ordertickethelper.h"

static CURL *curl;
static CURLcode res;
static struct curl_slist *host_list = NULL;
static struct response_data chunk;
struct common_list *all_stations;
struct common_list *cached_stations;
static struct curl_slist *nxt;
static struct passenger_info pinfo[16];
static struct passenger_info cur_passenger;
static struct ticket_info tinfo;
static struct screen_param scr;
struct user_config config;

extern int setup_mail(struct user_config *);
int main(int argc, char *argv[])
{
    if(signal(SIGINT, sig_handler) == SIG_ERR) {
	perror("signal: ");
    }
    if(signal(SIGTERM, sig_handler) == SIG_ERR) {
	perror("signal: ");
    }
    do_init();

    //init_user_screen();
    //init_curl();
    //init_buffer();

    /*clock_t t1, t2;
    t1 = clock();
    if(checkUserIsLogin() != 0) {
	user_login();
    }
    t2 = clock();
    printf("checkUserIsLogin time: %lu\n", t2 - t1);*/
    //show_varification_code();
    //query_ticket();

    do_cleanup();
    return 0;
}

int init_buffer()
{
    chunk.memory = (char *)malloc(1024 * 1024 * 4);  /* preallocate 4k memory */
    if(chunk.memory == NULL) {
	perror("malloc: ");
	return -1;
    }
    chunk.memory[0] = 0;
    chunk.size = 0;
    chunk.allocated_size = 1024 * 1024 * 4;
    return 0;
}

int init_curl()
{
    curl_global_init(CURL_GLOBAL_ALL);
    /* get a curl handle */
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#ifdef USE_COOKIE
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "./kyfw.12306.cn.cookie");
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "./kyfw.12306.cn.cookie");
#endif
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    host_list = curl_slist_append(NULL, "kyfw.12306.cn:443:183.61.26.197:443");
    //host_list = curl_slist_append(host_list, "kyfw.12306.cn:443:36.102.235.206:443");
    //host_list = curl_slist_append(host_list, "kyfw.12306.cn:443:27.148.151.214:443");
    nxt = host_list;

    //curl_easy_setopt(curl, CURLOPT_NOBODY, 1l);
    //perform_request("https://kyfw.12306.cn/otn/login/init", GET, NULL, host_list);
    return 0;
}

int do_init()
{
    init_buffer();
    init_curl();
    init_list(&all_stations);
    init_list(&cached_stations);

    load_stations_name(all_stations);
    load_config(&config);
    fill_user_config_telecode();
    print_config(&config);
    return 0;
}

int fill_user_config_telecode()
{
    struct common_list *fs_code, *ts_code;
    fs_code = find_node(cached_stations, config._from_station_name, find_station_code);
    if(fs_code == NULL) {
	fs_code = find_node(all_stations, config._from_station_name, find_station_code);
	insert_node(cached_stations, fs_code->data, insert_station_name);
    }
    //config._from_station_telecode = ((struct station_name *) fs_code->data)->code;
    strcpy(config._from_station_telecode, ((struct station_name *) fs_code->data)->code);

    ts_code = find_node(cached_stations, config._to_station_name, find_station_code);
    if(ts_code == NULL) {
	ts_code = find_node(all_stations, config._to_station_name, find_station_code);
	insert_node(cached_stations, ts_code->data, insert_station_name);
    }
    //config._to_station_telecode = ((struct station_name *) ts_code->data)->code;
    strcpy(config._to_station_telecode, ((struct station_name *) ts_code->data)->code);
    //printf("from_station_telecode: %s, to_station_telecode: %s\n", config._from_station_telecode, config._to_station_telecode);
    return 0;
}

int init_user_screen()
{
    initscr();
    getmaxyx(stdscr, scr.rows, scr.cols);
    scr.info = newwin(LINES / 10, COLS - 2, 0, 0);
    scr.output = newwin(LINES * 7 / 10 - 3, COLS - 2, LINES / 10 + 3, 0);
    scr.status = newwin(LINES * 2 / 10 - 3, COLS - 2, LINES * 8 / 10 + 3, 0);
    //scr.info = newwin(5, 10, 0, 0);
    //scr.output = newwin(10, 10, 8, 0);
    //scr.status = newwin(5, 10, 11, 0);
    wborder(scr.info, '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(scr.output, '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(scr.status, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(scr.info);
    wrefresh(scr.output);
    wrefresh(scr.status);
    return 0;
}

int destroy_win()
{
    /* box(local_win, ' ', ' '); : This won't produce the desired
     * result of erasing the window. It will leave it's four corners
     * and so an ugly remnant of window.
     */
    wborder(scr.info, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wborder(scr.output, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wborder(scr.status, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(scr.info);
    wrefresh(scr.output);
    wrefresh(scr.status);
    delwin(scr.info);
    delwin(scr.output);
    delwin(scr.status);

    return 0;
}

int clean_curl()
{
    curl_slist_free_all(host_list);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}

int do_cleanup()
{
    //destroy_win();
    //endwin();
    clean_curl();
    free(chunk.memory);
    //free_ptr_array((void **)s_name);
    clear_list(all_stations, remove_station_name);
    //clear_list(cached_stations, remove_station_name);
    return 0;
}

int parse_train_info(const char *s, char (*t_buffer)[512], struct train_info *ti)
{
    cJSON *response = cJSON_Parse(s);
    if(response == NULL) {
	return 1;
    }
    cJSON *status = cJSON_GetObjectItem(response, "status");
    if(status == NULL || !cJSON_IsBool(status)) {
	goto fail;
    }
    if(!cJSON_IsTrue(status)) {
	cJSON *msg = cJSON_GetObjectItem(response, "messages");
	if(!cJSON_IsNull(msg)) {
	    char *err_msg = cJSON_Print(msg);
	    printf("error: %s\n", cJSON_Print(msg));
	    //wprintw(scr.status, "error: %s\n", err_msg);
	    free(err_msg);
	}
	goto fail;
    }
    cJSON *data = cJSON_GetObjectItem(response, "data");
    if(data == NULL) {
	goto fail;
    }
    cJSON *flag = cJSON_GetObjectItem(data, "flag");
    if(flag == NULL || !cJSON_IsString(flag)) {
	goto fail;
    }
    if(strcmp(flag->valuestring, "1")) {
	goto fail;
    }
    cJSON *result = cJSON_GetObjectItem(data, "result");
    if(result == NULL || !cJSON_IsArray(result)) {
	goto fail;
    }
    int size = cJSON_GetArraySize(result);
    int i;
    struct common_list *fs_name;
    struct common_list *es_name;
    /*clock_t t1, t2;
    t1 = clock();
    struct train_info *t_info;
    int t_size = 64;
    t_info = (struct train_info *) malloc(t_size * sizeof(struct train_info));
    if(t_info == NULL) {
	perror("malloc: ");
	return NULL;
    }*/
    for(i = 0; i < size; i++) {
	cJSON *item = cJSON_GetArrayItem(result, i);
	if(cJSON_IsNull(item)) {
	    goto fail;
	}
	//printf("train info: %s\n", item->valuestring);
	strncpy(t_buffer[i], item->valuestring, 512);
	parse_peer_train(t_buffer[i], ti + i);
	fs_name = find_node(cached_stations, (ti + i)->from_station_telecode, find_station_name);
	if(fs_name == NULL) {
	    fs_name = find_node(all_stations, (ti + i)->from_station_telecode, find_station_name);
	    insert_node(cached_stations, fs_name->data, insert_station_name);
	}
	es_name = find_node(cached_stations, (ti + i)->to_station_telecode, find_station_name);
	if(es_name == NULL) {
	    es_name = find_node(all_stations, (ti + i)->to_station_telecode, find_station_name);
	    insert_node(cached_stations, es_name->data, insert_station_name);
	}
	(ti + i)->from_station_name = ((struct station_name *) fs_name->data)->name;
	(ti + i)->to_station_name = ((struct station_name *) es_name->data)->name;

	/*char **train = split(pitem + 1, '|');
	cJSON_free(pitem);
	if(train == NULL) {
	    return NULL;
	}
	if(i >= t_size - 1) {
	    t_info = (struct train_info *) realloc(t_info, t_size * sizeof(struct train_info) * 2);
	    if(t_info == NULL) {
		perror("realloc: ");
		free_ptr_array((void **)train);
		return NULL;
	    }
	    t_size += t_size;
	}
	memcpy(t_info + i, train, sizeof(struct train_info));
	//free_ptr_array((void **)train);
	free(train);*/
    }
    memset(ti + i, 0, sizeof(struct train_info));
    /*t2 = clock();
    printf("total object: %d\n", size);
    printf("spend time: %lu\n", t2 - t1);*/
    cJSON_Delete(response);
    return 0;
fail:
    cJSON_Delete(response);
    return 1;
}

static int
start_login()
{
    char url[64];
    char post_data[64];

    snprintf(url, sizeof(url), "%s", "https://kyfw.12306.cn/passport/web/login");
    snprintf(post_data, sizeof(post_data), "username=%s&password=%s&appid=otn", config._username, config._password);

    if(perform_request(url, POST, post_data, nxt) < 0) {
	return -1;
    }
    cJSON *ret_json = cJSON_Parse(chunk.memory);
    //char *str_json = cJSON_Print(ret_json);
    //printf("json: %s\n", str_json);
    if(cJSON_IsNull(ret_json)) {
	    fprintf(stderr, "error while parse json data.\n");
	    return -1;
    }
    cJSON *ret_code = cJSON_GetObjectItem(ret_json, "result_code");
    if(cJSON_IsNumber(ret_code) && ret_code->valueint == 0) {
	init_my12306();
	cJSON_Delete(ret_json);
	//free(str_json);
	return 0;
    }
    cJSON_Delete(ret_json);
    //free(str_json);
    return 1;
}

int show_varification_code(int auth_type)
{
    pthread_t tid;
    int *ret;
    //pthread_attr_t attr;
    //pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    struct thread_data tdata;
    tdata.curl = curl;
    tdata.auth_type = auth_type;
    tdata.r_data = &chunk;
    tdata.nxt = nxt;

    pthread_create(&tid, NULL, show_varification_code_main, (void *)&tdata);
    pthread_join(tid, (void **)&ret);

    int status = *ret;
    free(ret);
    return status;
}

int user_login()
{
    perform_request(BASEURL"otn/login/init", GET, NULL, nxt);

    int ret = show_varification_code(0);
    if(ret == 100) {
	if(start_login() == 0) {
	    printf("登陆成功\n");
	    //wprintw(scr.status, "登陆成功\n");
	    return 0;
	} else {
	    printf("登陆失败\n");
	    //wprintw(scr.status, "登陆失败\n");
	}
    } else {
	printf("验证码校验失败\n");
	//wprintw(scr.status, "验证码校验失败\n");
    }
    return 1;
}

int passenger_initdc()
{
    char url[64];
    char param[16];

    snprintf(url, sizeof(url), "%s", BASEURL"confirmPassenger/initDc");
    snprintf(param, sizeof(param), "%s", "_json_attr");

    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    char *p, *token_pos, *token_end_pos;
    if((p = strstr(chunk.memory, "globalRepeatSubmitToken")) != NULL) {
	if((token_pos = strchr(p, '\'')) != NULL) {
	    if((token_end_pos = strchr(token_pos + 1, '\'')) != NULL) {
		memcpy(tinfo.repeat_submit_token, token_pos + 1, token_end_pos - token_pos - 1);
		tinfo.repeat_submit_token[token_end_pos - token_pos - 1] = '\0';
	    }
	}
    } else {
	return 1;
    }
    p = chunk.memory + chunk.size * 2 / 3;
    //char *init_seatTypes, *defaultTicketTypes, *init_cardTypes, *ticket_seat_codeMap, *ticketInfoForPassengerForm, *orderRequestDTO;
    char *s_pos, *e_pos;
    char *ticketInfoForPassengerForm;
    size_t size, i;
    /*if((init_seatTypes = strstr(p, "init_seatTypes")) != NULL) {
	s_pos = strchr(init_seatTypes, '{');
	e_pos = strchr(init_seatTypes, ';');
	if(s_pos && e_pos) {
	    memcpy(tinfo.init_seatTypes, init_seatTypes, (size_t)(e_pos - s_pos));
	    tinfo.init_seatTypes[e_pos - s_pos] = '\0';
	}
    } else {
	return 1;
    }
    if((defaultTicketTypes = strstr(init_seatTypes, "defaultTicketTypes")) != NULL) {
	s_pos = strchr(defaultTicketTypes, '{');
	e_pos = strchr(defaultTicketTypes, ';');
	if(s_pos && e_pos) {
	    memcpy(tinfo.defaultTicketTypes, defaultTicketTypes, (size_t)(e_pos - s_pos));
	    tinfo.defaultTicketTypes[e_pos - s_pos] = '\0';
	}
    } else {
	return 1;
    }
    if((init_cardTypes = strstr(defaultTicketTypes, "init_cardTypes")) != NULL) {
	s_pos = strchr(init_cardTypes, '{');
	e_pos = strchr(init_cardTypes, ';');
	if(s_pos && e_pos) {
	    memcpy(tinfo.init_cardTypes, init_cardTypes, (size_t)(e_pos - s_pos));
	    tinfo.init_cardTypes[e_pos - s_pos] = '\0';
	}
    } else {
	return 1;
    }
    if((ticket_seat_codeMap = strstr(init_cardTypes, "ticket_seat_codeMap")) != NULL) {
	s_pos = strchr(ticket_seat_codeMap, '{');
	e_pos = strchr(ticket_seat_codeMap, ';');
	if(s_pos && e_pos) {
	    memcpy(tinfo.ticket_seat_codeMap, ticket_seat_codeMap, (size_t)(e_pos - s_pos));
	    tinfo.ticket_seat_codeMap[e_pos - s_pos] = '\0';
	}
    } else {
	return 1;
    }*/
    if((ticketInfoForPassengerForm = strstr(p, "ticketInfoForPassengerForm")) != NULL) {
	s_pos = strchr(ticketInfoForPassengerForm, '{');
	e_pos = strchr(ticketInfoForPassengerForm, ';');
	if(s_pos && e_pos) {
	    memcpy(tinfo.ticketInfoForPassengerForm, s_pos, (size_t)(e_pos - s_pos));
	    tinfo.ticketInfoForPassengerForm[e_pos - s_pos] = '\0';
	    size = e_pos - s_pos;
	    for(i = 0; i < size; i++) {
		if(tinfo.ticketInfoForPassengerForm[i] == '\'') {
		    tinfo.ticketInfoForPassengerForm[i] = '\"';  /* replace ' with \" then we can use cJSON parse it*/
		}
	    }
	    //printf("ticketInfoForPassengerForm: %s\n", tinfo.ticketInfoForPassengerForm);
	} else {
	    return 1;
	}
    } else {
	return 1;
    }
    /*if((orderRequestDTO = strstr(ticketInfoForPassengerForm, "orderRequestDTO")) != NULL) {
	s_pos = strchr(orderRequestDTO, '{');
	e_pos = strchr(orderRequestDTO, ';');
	if(s_pos && e_pos) {
	    memcpy(tinfo.orderRequestDTO, orderRequestDTO, (size_t)(e_pos - s_pos));
	    tinfo.orderRequestDTO[e_pos - s_pos] = '\0';
	}
    } else {
	return 1;
    }*/
    return 0;
}

int get_passenger_dtos(const char *repeat_token)
{
    char url[64];
    char par[64];

    snprintf(url, sizeof(url), "%s", BASEURL"confirmPassenger/getPassengerDTOs");
    snprintf(par, sizeof(par), "%s%s", "_json_attr=&REPEAT_SUBMIT_TOKEN=", repeat_token);

    if(perform_request(url, POST, par, nxt) < 0) {
	return -1;
    }

    //fprintf(stderr, "passengerDTOs: %s\n", chunk.memory);
    cJSON *root, *status, *httpstatus, *data;
    root = cJSON_Parse(chunk.memory);
    if(!root) {
	return 1;
    }
    status = cJSON_GetObjectItem(root, "status");
    httpstatus = cJSON_GetObjectItem(root, "httpstatus");
    if(!cJSON_IsTrue(status) || !cJSON_IsNumber(httpstatus) || httpstatus->valueint != 200) {
	cJSON_Delete(root);
	return 1;
    }
    data = cJSON_GetObjectItem(root, "data");
    if(!data) {
	cJSON_Delete(root);
	return 1;
    }
    cJSON *normal_passengers = cJSON_GetObjectItem(data, "normal_passengers");
    if(!normal_passengers) {
	cJSON_Delete(root);
	return 1;
    }
    int i, size;
    size = cJSON_GetArraySize(normal_passengers);
    /*pinfo = (struct passenger_info *) malloc(size * sizeof(struct passenger_info));
      if(!pinfo) {
      cJSON_Delete(root);
      return -1;
      }*/
    for(i = 0; i < size; i++) {
	cJSON *passenger = cJSON_GetArrayItem(normal_passengers, i);
	cJSON *param = cJSON_GetObjectItem(passenger, "code");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].code, param->valuestring, sizeof(pinfo->code));
	}
	param = cJSON_GetObjectItem(passenger, "passenger_name");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].passenger_name, param->valuestring, sizeof(pinfo->passenger_name));
	}
	param = cJSON_GetObjectItem(passenger, "sex_code");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].sex_code, param->valuestring, sizeof(pinfo->sex_code));
	}
	param = cJSON_GetObjectItem(passenger, "sex_name");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].sex_name, param->valuestring, sizeof(pinfo->sex_name));
	}
	param = cJSON_GetObjectItem(passenger, "born_date");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].born_date, param->valuestring, sizeof(pinfo->born_date));
	}
	param = cJSON_GetObjectItem(passenger, "country_code");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].country_code, param->valuestring, sizeof(pinfo->country_code));
	}
	param = cJSON_GetObjectItem(passenger, "passenger_id_type_code");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].passenger_id_type_code, param->valuestring, sizeof(pinfo->passenger_id_type_code));
	}
	param = cJSON_GetObjectItem(passenger, "passenger_id_type_name");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].passenger_id_type_name, param->valuestring, sizeof(pinfo->passenger_id_type_name));
	}
	param = cJSON_GetObjectItem(passenger, "passenger_id_no");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].passenger_id_no, param->valuestring, sizeof(pinfo->passenger_id_no));
	}
	param = cJSON_GetObjectItem(passenger, "passenger_type");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].passenger_type, param->valuestring, sizeof(pinfo->passenger_type));
	}
	param = cJSON_GetObjectItem(passenger, "passenger_flag");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].passenger_flag, param->valuestring, sizeof(pinfo->passenger_flag));
	}
	param = cJSON_GetObjectItem(passenger, "passenger_type_name");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].passenger_type_name, param->valuestring, sizeof(pinfo->passenger_type_name));
	}
	param = cJSON_GetObjectItem(passenger, "mobile_no");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].mobile_no, param->valuestring, sizeof(pinfo->mobile_no));
	}
	param = cJSON_GetObjectItem(passenger, "phone_no");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].phone_no, param->valuestring, sizeof(pinfo->phone_no));
	}
	param = cJSON_GetObjectItem(passenger, "email");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].email, param->valuestring, sizeof(pinfo->email));
	}
	param = cJSON_GetObjectItem(passenger, "address");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].address, param->valuestring, sizeof(pinfo->address));
	}
	param = cJSON_GetObjectItem(passenger, "postalcode");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].postalcode, param->valuestring, sizeof(pinfo->postalcode));
	}
	param = cJSON_GetObjectItem(passenger, "first_letter");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].first_letter, param->valuestring, sizeof(pinfo->first_letter));
	}
	param = cJSON_GetObjectItem(passenger, "recordCount");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].record_count, param->valuestring, sizeof(pinfo->record_count));
	}
	param = cJSON_GetObjectItem(passenger, "total_times");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].total_times, param->valuestring, sizeof(pinfo->total_times));
	}
	param = cJSON_GetObjectItem(passenger, "index_id");
	if(cJSON_IsString(param)) {
	    strncpy(pinfo[i].index_id, param->valuestring, sizeof(pinfo->index_id));
	}
	//cJSON_Delete(passenger);
    }
    memset(pinfo + i, 0, sizeof(struct passenger_info));
    cJSON_Delete(root);
    return 0;
}


void print_train_info(struct train_info *info)
{
    struct train_info *p = info;
    printf("%-8s%-14s%-12s%-12s%-12s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n", "车次", "出发站", "到达站", "出发时间", "到达时间", "历时", "特等座", "一等座", "二等座", "高级软卧", "软卧", "动卧", "硬卧", "软座", "硬座", "无座", "其他");
    //wprintw(scr.status, "车次\t出发站\t到达站\t出发时间\t到达时间\t历时\t特等座\t一等座\t二等座\t高级软卧\t软卧\t动卧\t硬卧\t软座\t硬座\t无座\t其他\n");
    while(p->train_no) {
	printf("%-8s%-12s%-12s%-12s%-12s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n", p->station_train_code, p->from_station_name, 
	//wprintw(scr.status, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", p->station_train_code, p->from_station_telecode, 
		p->to_station_name, p->start_time, p->arrive_time, p->spend_time, p->swz_num, p->zy_num, p->ze_num, p->gr_num, p->rw_num,
		p->yb_num, p->yw_num, p->rz_num, p->yz_num, p->wz_num, p->qt_num);
	p++;
    }
}

int perform_request(const char *url, enum request_type type, void *post, struct curl_slist *list)
{
    chunk.size = 0;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    if(list) {
	curl_easy_setopt(curl, CURLOPT_CONNECT_TO, list);
    }
    switch(type) {
	case GET:
	    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	    break;
	case POST:
	    curl_easy_setopt(curl, CURLOPT_POST, 1L);
	    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen((const char *)post));
	    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
	    break;
	default:
	    fprintf(stderr, "perform_request parameter: undefined request type.\n");
	    return -1;
    };
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Fedora; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36");

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
	fprintf(stderr, "curl_easy_peform() failed: %s\n", curl_easy_strerror(res));
	return -1;
    }
    return 0;
}

int check_current_train_has_prefix_ticket(struct train_info *ptrain)
{
    if(strstr(config._prefer_seat_type, "M") != NULL &&  ptrain->zy_num[0] != '\0') {
	return 0;
    } else if(strstr(config._prefer_seat_type, "O") != NULL &&  ptrain->ze_num[0] != '\0') {
	return 0;
    } else if(strstr(config._prefer_seat_type, "1") != NULL &&  ptrain->yz_num[0] != '\0') {
	return 0;
    } else {
	return 1;
    }
}

int check_current_train_submitable(struct train_info *ptrain)
{
    char tmp[2];
    tmp[0] = ptrain->station_train_code[0];
    tmp[1] = '\0';
    if(ptrain->can_web_buy[0] != 'Y') {
	return 1;
    }
    if(config._prefer_train_no[0] != '\0') {
	if(strstr(config._prefer_train_no, ptrain->station_train_code) == NULL) {
	    return 1;
	}
	if(check_current_train_has_prefix_ticket(ptrain) == 0) {
	    return 0;
	}
    }
    if(config._prefer_train_type[0] != 0) {
	if(strstr(config._prefer_train_type, tmp) == NULL) {
	    return 1;
	}
    }
    int i = 0;
    while(config._t_level[i].time_start[0] != 0) {
	if(strcmp(ptrain->start_time, config._t_level[i].time_start) >= 0 &&
		strcmp(ptrain->start_time, config._t_level[i].time_end) <= 0) {
	    break;
	}
	i++;
    }
    if(config._t_level[i].time_start[0] == 0 && i != 0) {
	return 1;
    }
    if(check_current_train_has_prefix_ticket(ptrain) == 0) {
	return 0;
    }
    return 1;
}

int process_train(struct train_info *ptrain)
{
    struct train_info *pt_info = ptrain;
    while(pt_info->train_no) {
	if(check_current_train_submitable(pt_info) == 0) {
	    if(submit_order_request(pt_info) == 0) {
		return 0;
	    }
	}
	pt_info++;
    }
    return 1;
}

/*int free_train_info(struct train_info *ptr)
{
    while(ptr->pbuff) {
	free(ptr->pbuff);
	ptr++;
    }
    return 0;
}*/

int query_ticket()
{
    char url[256];
    struct train_info t_info[512];
    char train_buffer[512][512];

    //curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);

    snprintf(url, sizeof(url), "%sleftTicket/log?leftTicketDTO.train_date=%s&leftTicketDTO.from_station=%s&leftTicketDTO.to_station=%s&purpose_codes=ADULT", BASEURL, config._start_tour_date, config._from_station_telecode, config._to_station_telecode);
    perform_request(url, GET, NULL, nxt);

    snprintf(url, sizeof(url), "%sleftTicket/query?leftTicketDTO.train_date=%s&leftTicketDTO.from_station=%s&leftTicketDTO.to_station=%s&purpose_codes=ADULT", BASEURL, config._start_tour_date, config._from_station_telecode, config._to_station_telecode);

    while(1) {
	if(perform_request(url, GET, NULL, nxt) < 0) {
	    sleep(3);
	    continue;
	}
	//nxt = nxt->next;
	/*if(nxt == NULL) {
	  nxt = host_list;
	  }*/
	parse_train_info(chunk.memory, train_buffer, t_info);
	//if(t_info) {
	print_train_info(t_info);
	if(process_train(t_info) == 0) {
	    return 0;
	}
	//free_ptr_array((void **)t_info);
	//}
	//free_train_info(t_info);
	sleep(config._query_ticket_interval);
    }
    //clear_list(cache);
    return 1;
}

int checkUserIsLogin()
{
    char url[64];
    char param[32];

    snprintf(url, sizeof(url), "%s", BASEURL"login/checkUser");
    snprintf(param, sizeof(param), "%s", "_json_att=");

    if(perform_request(url, POST, (void *)param, nxt) < 0) {
	return -1;
    }

    cJSON *status, *httpstatus, *data, *flag, *root;
    //printf("checkUser: %s\n", chunk.memory);

    root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }

    status = cJSON_GetObjectItem(root, "status");
    httpstatus = cJSON_GetObjectItem(root, "httpstatus");
    if(cJSON_IsTrue(status) && cJSON_IsNumber(httpstatus) && httpstatus->valueint == 200) {
	data = cJSON_GetObjectItem(root, "data");
	if(!cJSON_IsNull(data)) {
	    flag = cJSON_GetObjectItem(data, "flag");
	    if(cJSON_IsTrue(flag)) {
		return 0;
	    } else {
		return 1;
	    }
	}
    }
    return -1;
}

int init_my12306()
{
    char url[128];
    char param[64];

    //snprintf(url, sizeof(url), "%s", BASEURL"login/userLogin");
    //perform_request(url, POST, (void *)"_json_att=", nxt);
    snprintf(url, sizeof(url), "%s", BASEURL"passport?redirect=/otn/login/userLogin");
    perform_request(url, GET, NULL, nxt);
    snprintf(url, sizeof(url), "https://kyfw.12306.cn/passport/web/auth/uamtk");
    perform_request(url, POST, (void *)"appid=otn", nxt);
    //printf("umatk: %s\n", chunk.memory);

    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }

    cJSON *result_code = cJSON_GetObjectItem(root, "result_code");
    cJSON *newapptk;
    if(cJSON_IsNumber(result_code) && result_code->valueint == 0) {
	newapptk = cJSON_GetObjectItem(root, "newapptk");
	if(cJSON_IsString(newapptk)) {
	    snprintf(url, sizeof(url), "%s", BASEURL"uamauthclient");
	    snprintf(param, sizeof(param), "%s%s", "tk=", newapptk->valuestring);
	    perform_request(url, POST, param, nxt);
	    //printf("uamauthclient: %s\n", chunk.memory);
	}
    } else {
	return 1;
    }
    snprintf(url, sizeof(url), "%s", BASEURL"index/initMy12306");
    perform_request(url, GET, NULL, nxt);

    return 0;
}

int get_passenger_tickets_for_submit(cJSON *root, char *ptr, size_t size)
{
    cJSON *limitSeatTicketDTO = cJSON_GetObjectItem(root, "limitBuySeatTicketDTO");
    if(cJSON_IsNull(limitSeatTicketDTO)) {
	return 1;
    }

    cJSON *seat_type_codes = cJSON_GetObjectItem(limitSeatTicketDTO, "seat_type_codes");
    if(cJSON_IsNull(seat_type_codes)) {
	return 1;
    }
    cJSON *seat_obj = cJSON_GetArrayItem(seat_type_codes, 0);
    if(cJSON_IsNull(seat_obj)) {
	return 1;
    }
    int i;
    int array_size = cJSON_GetArraySize(seat_obj);
    for(i = 0; i < array_size; i++) {
	cJSON *id = cJSON_GetObjectItem(seat_obj, "id");
	if(!cJSON_IsString(id)) {
	    return 1;
	}
	if(strstr(config._prefer_seat_type, id->valuestring) != NULL) {
	    strncpy(tinfo.seat_type, id->valuestring, sizeof(tinfo.seat_type));
	    break;
	}
    }
    int num = snprintf(ptr, size, "%s,0,1,%s,1,%s,%s,N", tinfo.seat_type, cur_passenger.passenger_name, 
	    	cur_passenger.passenger_id_no, cur_passenger.mobile_no);
    return num;
}

int get_old_passenger_for_submit(char *ptr, size_t size)
{
    int num = snprintf(ptr, size, "%s,1,%s,1_", cur_passenger.passenger_name, cur_passenger.passenger_id_no);
    return num;
}

int set_cur_passenger()
{
    int i = 0;
    while(pinfo[i].code[0]) {
	if(strcmp(pinfo[i].passenger_name, config._passenger_name) == 0) {
	    break;
	}
	i++;
    }
    if(pinfo[i].code[0]) {
	memcpy(&cur_passenger, pinfo + i, sizeof(struct passenger_info));
    } else {
	return 1;
    }
    return 0;
}

int start_submit_order_request(struct train_info *t_info)
{
    char url[64];
    char param[512];
    time_t now;
    struct tm *ts;
    time(&now);
    ts = localtime(&now);
    char *p_stdate = t_info->start_train_date;

    snprintf(url, sizeof(url), "%s", BASEURL"leftTicket/submitOrderRequest");
    snprintf(param, sizeof(param), "%s%s%s%c%c%c%c-%c%c-%c%c%s%d-%d-%d%s%s%s%s%s", "secretStr=", t_info->secretStr, "&train_date=", *p_stdate, *(p_stdate + 1), *(p_stdate + 2), *(p_stdate + 3), *(p_stdate + 4), *(p_stdate + 5), *(p_stdate + 6), *(p_stdate + 7),
	    "&back_train_date=", ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,"&tour_flag=dc&purpose_codes=ADULT&query_from_station_name=", config._from_station_name, "&query_to_station_name=", config._to_station_name, "&undefined");

    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    //printf("result: %s\n", chunk.memory);
    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if(cJSON_IsTrue(status)) {
	return 0;
    } else {
	return 1;
    }
}


int check_order_info(cJSON *json)
{
    char url[64];
    char param[256];

    snprintf(url, sizeof(url), "%s", BASEURL"confirmPassenger/checkOrderInfo");
    char passenger[64], old_passenger[64];
    get_passenger_tickets_for_submit(json, passenger, sizeof(passenger));
    char *url_encode_passenger = curl_easy_escape(curl, passenger, strlen(passenger));
    get_old_passenger_for_submit(old_passenger, sizeof(old_passenger));
    char *url_encode_old_passenger = curl_easy_escape(curl, old_passenger, strlen(old_passenger));
    snprintf(param, sizeof(param), "%s%s%s%s%s%s", "cancel_flag=2&bed_level_order_num=000000000000000000000000000000&passengerTicketStr=", url_encode_passenger, "&oldPassengerStr=", url_encode_old_passenger, "&tour_flag=dc&randCode=&_json_att=&REPEAT_SUBMIT_TOKEN=", tinfo.repeat_submit_token);

    curl_free(url_encode_passenger);
    curl_free(url_encode_old_passenger);

    //printf("checkOrderInfo param: %s\n", param);
    //printf("passenger: %s old_passenger: %s\n", passenger, old_passenger);

    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    //printf("checkOrderInfo result: %s\n", chunk.memory);

    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    cJSON *status, *http_status, *data, *submit_status;
    status = cJSON_GetObjectItem(root, "status");
    http_status = cJSON_GetObjectItem(root, "httpstatus");
    if(cJSON_IsTrue(status) && cJSON_IsNumber(http_status) && http_status->valueint == 200) {
	data = cJSON_GetObjectItem(root, "data");
	if(!cJSON_IsNull(data)) {
	    submit_status = cJSON_GetObjectItem(data, "submitStatus");
	    if(!cJSON_IsTrue(submit_status)) {
		cJSON_Delete(root);
		return 1;
	    }
	    cJSON *ifShowPassCode = cJSON_GetObjectItem(data, "ifShowPassCode");
	    if(!cJSON_IsString(ifShowPassCode)) {
		cJSON_Delete(root);
		return 1;
	    }
	    if(ifShowPassCode->valuestring[0] == 'Y') {
		cJSON_Delete(root);
		return 2;
	    } else {
		cJSON_Delete(root);
		return 0;
	    }
	}
    }
    cJSON_Delete(root);
    return 1;
}

int get_left_ticket_str(cJSON *root)
{
    cJSON *left, *key, *train_location;
    left = cJSON_GetObjectItem(root, "leftTicketStr");
    if(!cJSON_IsString(left)) {
	return 1;
    }
    strncpy(tinfo.left_ticket, left->valuestring, sizeof(tinfo.left_ticket));
    key = cJSON_GetObjectItem(root, "key_check_isChange");
    if(!cJSON_IsString(key)) {
	return 1;
    }
    strncpy(tinfo.key_is_change, key->valuestring, sizeof(tinfo.key_is_change));
    train_location = cJSON_GetObjectItem(root, "train_location");
    if(!cJSON_IsString(train_location)) {
	return 1;
    }
    strncpy(tinfo.train_location, train_location->valuestring, sizeof(tinfo.train_location));
    return 0;
}


int get_train_date_format(cJSON *root, char *tf, size_t size)
{
    cJSON *orderRequestDTO = cJSON_GetObjectItem(root, "orderRequestDTO");
    if(cJSON_IsNull(orderRequestDTO)) {
	return 1;
    }
    cJSON *train_date, *date, *day, *year, *month, *hours, *minutes, *seconds, *timezoneOffset;
    train_date = cJSON_GetObjectItem(orderRequestDTO, "train_date");
    if(cJSON_IsNull(train_date)) {
	return 1;
    }
    struct tm time_format;
    date = cJSON_GetObjectItem(train_date, "date");
    if(cJSON_IsNumber(date)) {
	time_format.tm_mday = date->valueint;
    }
    day = cJSON_GetObjectItem(train_date, "day");
    if(cJSON_IsNumber(day)) {
	time_format.tm_wday = day->valueint;
    }
    month = cJSON_GetObjectItem(train_date, "month");
    if(cJSON_IsNumber(month)) {
	time_format.tm_mon = month->valueint;
    }
    year = cJSON_GetObjectItem(train_date, "year");
    if(cJSON_IsNumber(year)) {
	time_format.tm_year = year->valueint;
    }
    hours = cJSON_GetObjectItem(train_date, "hours");
    if(cJSON_IsNumber(hours)) {
	time_format.tm_hour = hours->valueint;
    }
    minutes = cJSON_GetObjectItem(train_date, "minutes");
    if(cJSON_IsNumber(minutes)) {
	time_format.tm_min = minutes->valueint;
    }
    seconds = cJSON_GetObjectItem(train_date, "seconds");
    if(cJSON_IsNumber(seconds)) {
	time_format.tm_sec = seconds->valueint;
    }
    int time_zone, tz_hh, tz_mm;
    timezoneOffset = cJSON_GetObjectItem(train_date, "timezoneOffset");
    if(cJSON_IsNumber(timezoneOffset)) {
	time_zone = timezoneOffset->valueint;
	tz_hh = time_zone / -60;
	tz_mm = time_zone % -60;
    }

    char buffer[64];
    if(strftime(buffer, sizeof(buffer), "%a %b %d %Y %T", &time_format) <= 0) {
	return -1;
    }
    int bf_len = strlen(buffer);
    char *pbf = buffer + bf_len;
    if(snprintf(pbf, sizeof(buffer) - bf_len, " GMT%s%02d%02d (CST)", tz_hh > 0 ? "+" : "-", tz_hh, tz_mm) <= 0) {
	return -1;
    }
    bf_len = strlen(buffer);
    int i, j;
    for(i = 0, j = 0; i < bf_len; i++, j++) {
	if(buffer[i] == ' ') {
	    tf[j] = '+';
	} else if(buffer[i] == ':') {
	    tf[j] = '%';
	    tf[j + 1] = '3';
	    tf[j + 2] = 'A';
	    j += 2;
	} else if(buffer[i] == '+') {
	    tf[j] = '%';
	    tf[j + 1] = '2';
	    tf[j + 2] = 'B';
	    j += 2;
	} else {
	    tf[j] = buffer[i];
	}
    }
    tf[j] = '\0';
    return 0;
}

int get_queue_count(cJSON *json, struct train_info *t_info)
{
    char date_format[64];
    char url[64], param[512];

    get_train_date_format(json, date_format, sizeof(date_format));
    //printf("date format: %s\n", date_format);
    //char *url_encode_train_date = curl_easy_escape(curl, date_format, strlen(date_format));
    
    snprintf(url, sizeof(url), "%s", BASEURL"confirmPassenger/getQueueCount");
    snprintf(param, sizeof(param), "train_date=%s&train_no=%s&stationTrainCode=%s&seatType=%s&fromStationTelecode=%s&toStationTelecode=%s&leftTicket=%s&purpose_codes=00&train_location=%s&_json_att=&REPEAT_SUBMIT_TOKEN=%s", 
	    date_format, t_info->train_no, 
	    t_info->station_train_code, tinfo.seat_type, 
	    t_info->from_station_telecode, t_info->to_station_telecode, 
	    tinfo.left_ticket, tinfo.train_location, 
	    tinfo.repeat_submit_token);
    //printf("queue param: %s %s\n", date_format, param);
    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    //printf("queue result: %s\n", chunk.memory);
    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if(!cJSON_IsTrue(status)) {
	cJSON_Delete(root);
	return 1;
    }
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if(cJSON_IsNull(data)) {
	cJSON_Delete(root);
	return 1;
    }
    //cJSON *count = cJSON_GetObjectItem(data, "count");
    cJSON *ticket_str = cJSON_GetObjectItem(data, "ticket");
    if(!cJSON_IsString(ticket_str)) {
	cJSON_Delete(root);
	return 1;
    }
    //long ticket_num = strtol(ticket_str->valuestring, NULL, 10);
    printf("当前余票：%s\n", ticket_str->valuestring);
    //wprintw(scr.status, "当前余票：%s\n", ticket_str->valuestring);
    cJSON *countT_str = cJSON_GetObjectItem(data, "countT");
    if(!cJSON_IsString(countT_str)) {
	cJSON_Delete(root);
	return 1;
    }
    printf("当前排队人数：%s\n", countT_str->valuestring);
    //wprintw(scr.status, "当前排队人数：%s\n", countT_str->valuestring);
    cJSON *op2_str = cJSON_GetObjectItem(data, "op_2");
    if(!cJSON_IsString(op2_str)) {
	cJSON_Delete(root);
	return 1;
    }
    if(strcmp(op2_str->valuestring, "true") == 0) {
	printf("当前排队人数超过余票张数，订单提交失败\n");
	//wprintw(scr.status, "当前排队人数超过余票张数，订单提交失败\n");
	return 2;
    }

    return 0;
}

int confirm_single_queue(cJSON *json)
{
    char url[128];
    char param[512];
    char passenger[64], old_passenger[64];

    snprintf(url, sizeof(url), "%s", BASEURL"confirmPassenger/confirmSingleForQueue");
    //printf("confirm_queue url: %s\n", url);

    get_passenger_tickets_for_submit(json, passenger, sizeof(passenger));
    get_old_passenger_for_submit(old_passenger, sizeof(old_passenger));

    char *url_encode_passenger = curl_easy_escape(curl, passenger, strlen(passenger));
    char *url_encode_old_passenger = curl_easy_escape(curl, old_passenger, strlen(old_passenger));

    snprintf(param, sizeof(param), "passengerTicketStr=%s&oldPassengerStr=%s&randCode=&purpose_codes=00&key_check_isChange=%s&leftTicketStr=%s&train_location=%s&choose_seats=&seatDetailType=000&roomType=00&dwAll=N&_json_att=&REPEAT_SUBMIT_TOKEN=%s", url_encode_passenger, url_encode_old_passenger,
	    tinfo.key_is_change, tinfo.left_ticket, tinfo.train_location,
	    tinfo.repeat_submit_token);
    curl_free(url_encode_passenger);
    curl_free(url_encode_old_passenger);
    //printf("confirmPassengerSingleForQueue param: %s\n", param);
    //printf("passenger: %s\n", passenger);
    //printf("old_passenger: %s\n", old_passenger);

    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    //printf("confirmPassengerSingleForQueue result: %s\n", chunk.memory);
    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if(!cJSON_IsTrue(status)) {
	cJSON_Delete(root);
	return 1;
    }
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if(cJSON_IsNull(data)) {
	return 1;
    }
    cJSON *submit_status = cJSON_GetObjectItem(data, "submitStatus");
    if(!cJSON_IsTrue(submit_status)) {
	cJSON *err_msg = cJSON_GetObjectItem(data, "errMsg");
	if(cJSON_IsString(err_msg)) {
	    printf("出票失败，原因：%s\n", err_msg->valuestring);
	    //wprintw(scr.status, "出票失败，原因：%s\n", err_msg->valuestring);
	}
	return 1;
    }
    return 0;
}

int query_order_wait_time()
{
    char url[256];

    snprintf(url, sizeof(url), "%s?random=%lu&tourFlag=dc&_json_att=&REPEAT_SUBMIT_TOKEN=%s", BASEURL"confirmPassenger/queryOrderWaitTime", time(NULL), tinfo.repeat_submit_token); 

    if(perform_request(url, GET, NULL, nxt) < 0) {
	return -1;
    }
    //printf("queryOrderWaitTime result: %s\n", chunk.memory);
    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if(cJSON_IsNull(data)) {
	cJSON_Delete(root);
	return 1;
    }
    cJSON *query_status = cJSON_GetObjectItem(data, "queryOrderWaitTimeStatus");
    if(cJSON_IsFalse(query_status)) {
	return 2;
    }
    cJSON *waitTime = cJSON_GetObjectItem(data, "waitTime");
    if(!cJSON_IsNumber(waitTime)) {
	return 1;
    }
    cJSON *order_id = cJSON_GetObjectItem(data, "orderId");
    if(cJSON_IsString(order_id)) {
	strncpy(tinfo.order_no, order_id->valuestring, sizeof(tinfo.order_no));
    }
    if(waitTime->valueint == -1 || waitTime->valueint == -100) {
	return 0;
    }
    return 3;
}

int result_order_for_dc_queue(const char *order_no)
{
    char url[128];
    char param[256];

    snprintf(url, sizeof(url), BASEURL"confirmPassenger/resultOrderForDcQueue");
    snprintf(param, sizeof(param), "orderSequence_no=%s&_json_att=&REPEAT_SUBMIT_TOKEN=%s", order_no, tinfo.repeat_submit_token);

    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    printf("resultOrderForDcQueue result: %s\n", chunk.memory);
    return 0;
}

int submit_order_request(struct train_info *t_info)
{
    struct train_info *ptrain = t_info;
    printf("开始提交订单请求...\n");
    if(checkUserIsLogin() != 0 && user_login() != 0) {
	return 1;
    }
    printf("正在预提交订单请求...\n");
    if(start_submit_order_request(ptrain) == 0) {
	printf("正在获取预提交令牌环...\n");
	passenger_initdc();
	cJSON *root = cJSON_Parse(tinfo.ticketInfoForPassengerForm);
	if(cJSON_IsNull(root)) {
	    return 1;
	}
	printf("正在获取乘车人信息...\n");
	get_passenger_dtos(tinfo.repeat_submit_token);
	set_cur_passenger();
	get_left_ticket_str(root);

	printf("正在检查预提交订单...\n");
	int ret = check_order_info(root);
	if(ret != 0) {
	    printf("当前需要验证码\n");
	    if(show_varification_code(1) != 0) {
		cJSON_Delete(root);
		return 1;
	    }
	} else {
	    printf("当前无需验证码，继续提交订单...\n");
	}
	printf("正在获取排队人数...\n");
	if(get_queue_count(root, ptrain) != 0) {
	    cJSON_Delete(root);
	    return 1;
	}
	printf("正在确认订单请求...\n");
	if(confirm_single_queue(root) != 0) {
	    cJSON_Delete(root);
	    return 1;
	} /*else {
	    return 0;
	}*/
	int wait_time;
	printf("系统出票中，开始等待...\n");
	while((wait_time = query_order_wait_time()) == 3) {
	    printf("继续等待系统出票...\n");
	    sleep(3);
	}
	if(wait_time != 0) {
	    cJSON_Delete(root);
	    return 1;
	}
	printf("系统出票成功，订单已完成\n");
	result_order_for_dc_queue(tinfo.order_no);
	cJSON_Delete(root);
	return 0;
    }
    return 1;
}

size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct response_data *mem = (struct response_data *)userp;

    if((mem->allocated_size - mem->size) < realsize) {
        mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1 + 1024 * 1024 * 4);  /* we allocate extra 4k memory */
	    if(mem->memory == NULL) {
	        fprintf(stderr, "not enough memory (realloc returned NULL)\n");
	        return 0;
	    }
	    mem->allocated_size = mem->size + realsize + 1 + 1024 * 1024 * 4;
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static void sig_handler(int signo)
{
    if(signo == SIGINT || signo == SIGTERM) {
	do_cleanup();
	exit(1);
    }
}
