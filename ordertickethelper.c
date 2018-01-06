#include "ordertickethelper.h"

extern char rand_code[128];

int main(int argc, char *argv[])
{
    int c;
    while((c = getopt_long(argc, argv, "c:hqQvV", long_options, NULL)) != -1) {
	switch(c) {
	    case 'c':
		strncpy(cmd_opt.config_file, optarg, sizeof(cmd_opt.config_file));
		break;
	    case 'h':
		print_help();
		break;
	    case 'q':
		cmd_opt.queit = true;
		break;
	    case 'Q':
		cmd_opt.query_only = true;
		break;
	    case 'v':
		cmd_opt.verbose = true;
		break;
	    case 'V':
		print_app_version();
		break;
	    case '?':
		exit(1);
	    default:
		break;
	}
    }
    if(optind < argc) {
	printf("invalid argument: ");
	while(optind < argc) {
	    printf("%s ", argv[optind++]);
	}
	printf("\n");
    }

    if(signal(SIGINT, sig_handler) == SIG_ERR) {
	perror("signal: ");
    }
    if(signal(SIGTERM, sig_handler) == SIG_ERR) {
	perror("signal: ");
    }
    if(do_init() < 0) {
	fprintf(stderr, "initialize failed.\n");
	do_cleanup();
	return 1;
    }

    if(!cmd_opt.query_only && check_user() != 0) {
	printf("当前用户未登陆，开始登陆...\n");
	if(user_login() != 0) {
	    do_cleanup();
	    exit(1);
	}
    }
    query_ticket();

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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Fedora; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36");
    if(cmd_opt.verbose) {
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }
#ifdef USE_COOKIE
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "./kyfw.12306.cn.cookie");
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "./kyfw.12306.cn.cookie");
#endif
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    return 0;
}

int do_init()
{
    init_buffer();
    init_curl();
    init_list(&all_stations);
    init_list(&cached_stations);
    init_list(&black_list);

    printf("正在加载站点列表...\n");
    if(load_stations_name(all_stations) != 0) {
	do_cleanup();
	exit(1);
    }

    printf("正在加载配置文件...\n");
    if(load_config(&config, cmd_opt.config_file) < 0) {
	fprintf(stderr, "load configuration fail, check is tickethelper.conf exist.\n");
	return -1;
    }
    if(!current_config_is_correct()) {
	return -1;
    }

    fill_user_config_telecode();
    int i = 0;
    while(config._passenger_name[i][0]) {
	config._passenger_count++;
	i++;
    }

    if(cmd_opt.verbose) {
	print_config(&config);
    }
    if(config._use_cdn_server_file[0]) {
	printf("正在加载cdn服务器列表...\n");
	load_cdn_server(&host_list, config._use_cdn_server_file);
	if(cmd_opt.verbose) {
	    print_cdn_server(host_list);
	}
	nxt = host_list;
    }
    printf("初始化完成\n");
    return 0;
}

void print_app_version()
{
    printf(APPVERSION"\n");
    exit(0);
}

int fill_user_config_telecode()
{
    struct common_list *fs_code, *ts_code;
    fs_code = find_node(cached_stations, config._from_station_name, find_station_code);
    if(fs_code == NULL) {
	fs_code = find_node(all_stations, config._from_station_name, find_station_code);
	if(fs_code == NULL) {
	    fprintf(stderr, "No such station: %s, perhaps you need to update station_name.txt\n", config._from_station_name);
	    do_cleanup();
	    exit(2);
	}
	insert_node(cached_stations, fs_code->data, insert_station_name);
    }
    strncpy(config._from_station_telecode, ((struct station_name *) fs_code->data)->code, sizeof(config._from_station_telecode));

    ts_code = find_node(cached_stations, config._to_station_name, find_station_code);
    if(ts_code == NULL) {
	ts_code = find_node(all_stations, config._to_station_name, find_station_code);
	if(ts_code == NULL) {
	    fprintf(stderr, "No such station: %s, perhaps you need to update station_name.txt\n", config._to_station_name);
	    do_cleanup();
	    exit(2);
	}
	insert_node(cached_stations, ts_code->data, insert_station_name);
    }
    strncpy(config._to_station_telecode, ((struct station_name *) ts_code->data)->code, sizeof(config._to_station_telecode));
    return 0;
}

bool current_config_is_correct()
{
    if(config._username[0] == 0) {
	printf("error: username not set,this field is required.\n");
	return false;
    }
    if(config._password[0] == 0) {
	printf("error: password not set,this field is required.\n");
	return false;
    }
    if(config._start_tour_date[0] == 0) {
	printf("error: start_tour_date not set,this field is required.\n");
	return false;
    }
    if(config._from_station_name[0] == 0) {
	printf("error: from_station_name not set,this field is required.\n");
	return false;
    }
    if(config._to_station_name[0] == 0) {
	printf("error: to_station_name not set,this field is required.\n");
	return false;
    }
    if(config._passenger_name[0][0] == 0) {
	printf("error: passenger_name not set,this field is required.\n");
	return false;
    }
    return true;
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
    clean_curl();
    free(chunk.memory);
    clear_list(all_stations, remove_station_name);
    clear_list(cached_stations, remove_station_name);
    clear_list(black_list, remove_black_list);
    return 0;
}

int parse_train_info(cJSON *response, struct train_info *ti)
{
    cJSON *status = cJSON_GetObjectItem(response, "status");
    if(!cJSON_IsTrue(status)) {
	cJSON *msg = cJSON_GetObjectItem(response, "messages");
	if(!cJSON_IsNull(msg)) {
	    char *err_msg = cJSON_Print(msg);
	    printf("error: %s\n", cJSON_Print(msg));
	    free(err_msg);
	}
	return 1;
    }
    cJSON *data = cJSON_GetObjectItem(response, "data");
    if(cJSON_IsNull(data)) {
	return 1;
    }
    cJSON *flag = cJSON_GetObjectItem(data, "flag");
    if(!cJSON_IsString(flag)) {
	return 1;
    }
    if(strcmp(flag->valuestring, "1")) {
	return 1;
    }
    cJSON *result = cJSON_GetObjectItem(data, "result");
    if(!cJSON_IsArray(result)) {
	return 1;
    }
    int size = cJSON_GetArraySize(result);
    int i;
    struct common_list *fs_name;
    struct common_list *es_name;

    for(i = 0; i < size; i++) {
	cJSON *item = cJSON_GetArrayItem(result, i);
	if(!cJSON_IsString(item)) {
	    return 1;
	}
	parse_peer_train(item->valuestring, ti + i);
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
    }
    memset(ti + i, 0, sizeof(struct train_info));
    return 0;
}

static int
start_login()
{
    char url[64];
    char post_data[256];

    //snprintf(url, sizeof(url), TARGETDOMAIN"passport/web/login");
    //snprintf(post_data, sizeof(post_data), "username=%s&password=%s&appid=otn", config._username, config._password);
    snprintf(url, sizeof(url), BASEURL"login/loginAysnSuggest");
    snprintf(post_data, sizeof(post_data), "loginUserDTO.user_name=%s&userDTO.password=%s&randCode=%s", 
	    config._username, config._password, rand_code);

    if(perform_request(url, POST, post_data, nxt) < 0) {
	return -1;
    }
    cJSON *ret_json = cJSON_Parse(chunk.memory);
    printf("login result: %s\n", chunk.memory);
    if(cJSON_IsNull(ret_json)) {
	return 1;
    }
    //cJSON *ret_code = cJSON_GetObjectItem(ret_json, "result_code");
    cJSON *status = cJSON_GetObjectItem(ret_json, "status");
    if(cJSON_IsTrue(status)) {
	cJSON *data = cJSON_GetObjectItem(ret_json, "data");
	if(cJSON_IsNull(data)) {
	    cJSON_Delete(ret_json);
	    return 1;
	}
	cJSON *login_check = cJSON_GetObjectItem(data, "loginCheck");
	if(cJSON_IsString(login_check) && strcmp(login_check->valuestring, "Y") == 0) {
	    init_my12306();
	    cJSON_Delete(ret_json);
	    return 0;
	} else {
	    cJSON *errMsg = cJSON_GetObjectItem(data, "otherMsg");
	    if(cJSON_IsString(errMsg)) {
		printf("error: %s\n", errMsg->valuestring);
	    }
	}
    }
    cJSON_Delete(ret_json);
    return 1;
}

int show_varification_code(int auth_type)
{
    pthread_t tid;
    int *ret;
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
    perform_request(BASEURL"login/init", GET, NULL, nxt);

    int ret = show_varification_code(0);
    if(ret == 100) {
	if(start_login() == 0) {
	    printf("登陆成功\n");
	    return 0;
	} else {
	    printf("登陆失败\n");
	}
    } else {
	printf("验证码校验失败\n");
    }
    return 1;
}

int add_train_to_black_list(struct train_info *t_info)
{
    time_t tvnow;
    time(&tvnow);
    struct train_black_list bl;
    bl.train_no = t_info->train_no;
    bl.block_time_end = tvnow + (time_t)config._block_time;
    insert_node(black_list, &bl, insert_black_list);
    return 0;
}

int remove_train_from_black_list(void *s)
{
    return remove_node(black_list, s, find_and_remove_black_list);
}

int is_need_to_remove_train_from_black_list()
{
    struct common_list *p = black_list->next;
    time_t tvnow;
    time(&tvnow);

    while(p) {
	if(tvnow >= ((struct train_black_list *) p->data)->block_time_end) {
	    //printf("remove %s from black list\n", ((struct train_black_list *) p->data)->train_no);
	    remove_train_from_black_list(((struct train_black_list *)p->data)->train_no);
	}
	p = p->next;
    }
    return 0;
}

bool current_train_is_at_black_list(struct train_info *t_info)
{
    if(find_node(black_list, t_info->train_no, find_black_list) == NULL) {
	return false;
    }
    return true;
}


int passenger_initdc()
{
    char url[64];
    char param[16];

    snprintf(url, sizeof(url), BASEURL"confirmPassenger/initDc");
    snprintf(param, sizeof(param), "_json_attr");

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
    char *s_pos, *e_pos;
    char *ticketInfoForPassengerForm;
    size_t size, i;
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
	} else {
	    return 1;
	}
    } else {
	return 1;
    }
    return 0;
}

int get_passenger_dtos(const char *repeat_token)
{
    char url[64];
    char par[64];

    snprintf(url, sizeof(url), BASEURL"confirmPassenger/getPassengerDTOs");
    snprintf(par, sizeof(par), "_json_attr=&REPEAT_SUBMIT_TOKEN=%s", repeat_token);

    if(perform_request(url, POST, par, nxt) < 0) {
	return -1;
    }

    cJSON *root, *status, *data;
    root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    status = cJSON_GetObjectItem(root, "status");
    if(!cJSON_IsTrue(status)) {
	goto failed;
    }
    data = cJSON_GetObjectItem(root, "data");
    if(cJSON_IsNull(data)) {
	goto failed;
    }
    cJSON *normal_passengers = cJSON_GetObjectItem(data, "normal_passengers");
    if(cJSON_IsNull(normal_passengers)) {
	goto failed;
    }
    int i, j, size;
    size = cJSON_GetArraySize(normal_passengers);
    const unsigned long p_offset[] = {
	SMOFFSET(struct passenger_info,code), SMOFFSET(struct passenger_info,passenger_name), SMOFFSET(struct passenger_info,sex_code),
	SMOFFSET(struct passenger_info,sex_name), SMOFFSET(struct passenger_info,born_date),SMOFFSET(struct passenger_info,country_code), 
	SMOFFSET(struct passenger_info,passenger_id_type_code), SMOFFSET(struct passenger_info,passenger_id_type_name),
	SMOFFSET(struct passenger_info,passenger_id_no), SMOFFSET(struct passenger_info,passenger_type), SMOFFSET(struct passenger_info,passenger_flag),
	SMOFFSET(struct passenger_info,passenger_type_name), SMOFFSET(struct passenger_info,mobile_no), SMOFFSET(struct passenger_info,phone_no),
	SMOFFSET(struct passenger_info,email), SMOFFSET(struct passenger_info,address), SMOFFSET(struct passenger_info,postalcode),
	SMOFFSET(struct passenger_info,first_letter), SMOFFSET(struct passenger_info,record_count), SMOFFSET(struct passenger_info,total_times),
	SMOFFSET(struct passenger_info,index_id)
    };
    const char * const index_name[] = {
	"code", "passenger_name", "sex_code", "sex_name", "born_date",
	"country_code", "passenger_id_type_code", "passenger_id_type_name", "passenger_id_no", "passenger_type",
	"passenger_flag", "passenger_type_name", "mobile_no", "phone_no", "email",
	"address", "postalcode", "first_letter", "recordCount", "total_times",
	"index_id", NULL
    };

    for(i = 0; i < size; i++) {
	cJSON *passenger = cJSON_GetArrayItem(normal_passengers, i);
	for(j = 0; index_name[j]; j++) {
	    cJSON *param = cJSON_GetObjectItem(passenger, index_name[j]);
	    if(cJSON_IsString(param)) {
		strcpy(((*pinfo).code + i * sizeof(struct passenger_info) + p_offset[j]), param->valuestring);
			//index_name[j + 1] ? p_offset[j + 1] - p_offset[j] : sizeof(struct passenger_info) - p_offset[j]);
	    }
	}
	/*cJSON *param = cJSON_GetObjectItem(passenger, "code");
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
	}*/
    }
    memset(pinfo + i, 0, sizeof(struct passenger_info));
    cJSON_Delete(root);
    return 0;
failed:
    cJSON_Delete(root);
    return 1;
}

void print_train_info(struct train_info *info)
{
    struct train_info *p = info;
    printf("+--------------------------------------------------------------------------------------------------------------------------+\n");
    printf("|%-8s|%-10s|%-10s|%-12s|%-12s|%-8s|%-10s|%-10s|%-10s|%-12s|%-7s|%-7s|%-7s|%-7s|%-7s|%-7s|%-7s|\n", "车次", "出发站", "到达站", "出发时间", "到达时间", "历时", "特等座", "一等座", "二等座", "高级软卧", "软卧", "动卧", "硬卧", "软座", "硬座", "无座", "其他");
    while(p->train_no) {
	printf("|------|-------|-------|--------|--------|------|-------|-------|-------|--------|-----|-----|-----|-----|-----|-----|-----|\n");
	printf("|%-6s|%-*s|%-*s|%-8s|%-8s|%-6s|%-*s|%-*s|%-*s|%-*s|%-*s|%-*s|%-*s|%-*s|%-*s|%-*s|%-*s|\n", p->station_train_code, 7 + (int)strlen(p->from_station_name) / 3, p->from_station_name, 
		7 + (int)strlen(p->to_station_name) / 3, p->to_station_name, p->start_time, p->arrive_time, p->spend_time, 7 + (int)strlen(p->swz_num) / 3, p->swz_num,  7 + (int)strlen(p->zy_num) / 3, p->zy_num, 7 + (int)strlen(p->ze_num) / 3, p->ze_num, 8 + (int)strlen(p->gr_num) / 3, p->gr_num, 5 + (int)strlen(p->rw_num) / 3, p->rw_num,
		5 + (int)strlen(p->yb_num) / 3, p->yb_num, 5 + (int)strlen(p->yw_num) / 3, p->yw_num, 5 + (int)strlen(p->rz_num) / 3, p->rz_num, 5 + (int)strlen(p->yz_num) / 3, p->yz_num, 5 + (int)strlen(p->wz_num) / 3, p->wz_num, 5 + (int)strlen(p->qt_num) / 3, p->qt_num);
	p++;
    }
    printf("+--------------------------------------------------------------------------------------------------------------------------+\n");
}

int perform_request(const char *url, enum request_type type, void *post, struct curl_slist *list)
{
    chunk.size = 0;
    curl_easy_setopt(curl, CURLOPT_URL, url);
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

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
	fprintf(stderr, "curl_easy_peform() failed: %s\n", curl_easy_strerror(res));
	return -1;
    }
    return 0;
}

bool current_train_has_prefix_ticket(struct train_info *ptrain)
{
    if(config._prefer_seat_type_all[0] == 0) {
	return true;
    } else if(strstr(config._prefer_seat_type_all, "M") != NULL &&  (ptrain->zy_num[0] != '\0' && strcmp(ptrain->zy_num, "无"))) {
	return true;
    } else if(strstr(config._prefer_seat_type_all, "O") != NULL &&  (ptrain->ze_num[0] != '\0' && strcmp(ptrain->ze_num, "无"))) {
	return true;
    } else if(strstr(config._prefer_seat_type_all, "1") != NULL &&  ptrain->yz_num[0] != '\0') {
	return true;
    } else if(strstr(config._prefer_seat_type_all, "3") != NULL &&  ptrain->yw_num[0] != '\0') {
	return true;
    } else if(strstr(config._prefer_seat_type_all, "4") != NULL &&  ptrain->rw_num[0] != '\0') {
	return true;
    } else if(strstr(config._prefer_seat_type_all, "5") != NULL &&  (ptrain->wz_num[0] != '\0' && strcmp(ptrain->wz_num, "无"))) {
	return true;
    } else if(strstr(config._prefer_seat_type_all, "A") != NULL &&  ptrain->qt_num[0] != '\0') {
	return true;
    } else if(strstr(config._prefer_seat_type_all, "P") != NULL &&  ptrain->swz_num[0] != '\0') {
	return true;
    } else {
	return false;
    }
}

int process_prefix_train_no(struct train_info *ptrain)
{
    struct train_info *pt = ptrain;
    int i = 0;
    while(config._prefer_train_no[i][0]) {
	pt = ptrain;
	while(pt->train_no) {
	    if(pt->can_web_buy[0] == 'Y' &&
		    strcmp(config._prefer_train_no[i], pt->station_train_code) == 0 &&
		    current_train_has_prefix_ticket(pt) &&
		    !current_train_is_at_black_list(pt)) {
		if(submit_order_request(pt) == 0) {
		    return 0;
		} else {
		    sleep(1);
		}
	    }
	    pt++;
	}
	i++;
    }
    return 1;
}

bool current_train_is_at_prefix_time(struct train_info *ptrain)
{
    if(config._t_level[0].time_start[0] == 0) {
	return true;
    }
    int i = 0;
    while(config._t_level[i].time_start[0] != 0) {
	if(strcmp(ptrain->start_time, config._t_level[i].time_start) >= 0 && 
		strcmp(ptrain->start_time, config._t_level[i].time_end) <= 0) {
	    return true;
	}
	i++;
    }
    return false;
}


int process_prefix_train_type(struct train_info *ptrain)
{
    struct train_info *pt = ptrain;
    int i = 0;
    while(config._prefer_train_type[i][0]) {
	pt = ptrain;
	while(pt->train_no) {
	    if(pt->can_web_buy[0] == 'Y' &&
		    config._prefer_train_type[i][0] == pt->station_train_code[0] &&
		    current_train_is_at_prefix_time(pt) &&
		    current_train_has_prefix_ticket(pt) &&
		    !current_train_is_at_black_list(pt)) {
		if(submit_order_request(pt) == 0) {
		    return 0;
		} else {
		    sleep(1);
		}
	    }
	    pt++;
	}
	i++;
    }
    return 1;
}

int process_not_prefix_train(struct train_info *ptrain)
{
    struct train_info *pt = ptrain;
    while(pt->train_no) {
	if(pt->can_web_buy[0] == 'Y' &&
		current_train_is_at_prefix_time(pt) &&
		current_train_has_prefix_ticket(pt) &&
		!current_train_is_at_black_list(pt)) {
	    if(submit_order_request(pt) == 0) {
		return 0;
	    } else {
		sleep(1);
	    }
	}
	pt++;
    }
    return 1;
}

int process_train(struct train_info *ptrain)
{
    struct train_info *pt_info = ptrain;
    int ret_val;
    if(config._prefer_train_no[0][0]) {
	ret_val = process_prefix_train_no(pt_info);
    } else if(config._prefer_train_type[0][0]) {
	ret_val = process_prefix_train_type(pt_info);
    } else {
	ret_val = process_not_prefix_train(pt_info);
    }
    return ret_val;
}

int query_ticket()
{
    char log_url[256];
    char query_url[256];
    struct train_info t_info[1024];
    struct timespec t;

    t.tv_sec = config._query_ticket_interval / 1000;
    t.tv_nsec = config._query_ticket_interval % 1000 * 1000000;

    snprintf(log_url, sizeof(log_url), "%sleftTicket/log?leftTicketDTO.train_date=%s&leftTicketDTO.from_station=%s&leftTicketDTO.to_station=%s&purpose_codes=ADULT", BASEURL, config._start_tour_date, config._from_station_telecode, config._to_station_telecode);

    snprintf(query_url, sizeof(query_url), "%sleftTicket/queryA?leftTicketDTO.train_date=%s&leftTicketDTO.from_station=%s&leftTicketDTO.to_station=%s&purpose_codes=ADULT", BASEURL, config._start_tour_date, config._from_station_telecode, config._to_station_telecode);

    while(1) {
	printf("正在查询 %s 从 %s 到 %s 的余票信息...\n", config._start_tour_date, config._from_station_name, config._to_station_name);
	perform_request(log_url, GET, NULL, nxt);
	if(perform_request(query_url, GET, NULL, nxt) < 0) {
	    nanosleep(&t, NULL);
	    if(config._use_cdn_server_file[0]) {
		nxt = nxt->next;
		if(nxt == NULL) {
		    nxt = host_list;
		}
	    }
	    continue;
	}
	if(config._use_cdn_server_file[0]) {
	    nxt = nxt->next;
	    if(nxt == NULL) {
		nxt = host_list;
	    }
	}
	cJSON *root = cJSON_Parse(chunk.memory);
	if(cJSON_IsNull(root)) {
	    nanosleep(&t, NULL);
	    continue;
	}
	is_need_to_remove_train_from_black_list();
	if(parse_train_info(root, t_info) != 0) {
	    nanosleep(&t, NULL);
	    cJSON_Delete(root);
	    continue;
	}
	cmd_opt.queit ? 0 : print_train_info(t_info);
	if(!cmd_opt.query_only && process_train(t_info) == 0) {
	    cJSON_Delete(root);
	    return 0;
	}
	cJSON_Delete(root);
	nanosleep(&t, NULL);
    }
    return 1;
}

int check_user_is_login()
{
    char url[64];
    char param[32];

    snprintf(url, sizeof(url), TARGETDOMAIN"passport/web/auth/uamtk");
    snprintf(param, sizeof(param), "appid=otn");

    if(perform_request(url, POST, (void *)param, nxt) < 0) {
	return -1;
    }

    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }

    cJSON *result_code = cJSON_GetObjectItem(root, "result_code");
    if(cJSON_IsNumber(result_code) && result_code->valueint == 0) {
	cJSON_Delete(root);
	return 0;
    } else {
	cJSON *msg = cJSON_GetObjectItem(root, "result_message");
	if(cJSON_IsString(msg)) {
	    printf("notice: %s\n", msg->valuestring);
	}
    }
    cJSON_Delete(root);
    return 1;
}

int check_user()
{
    char url[64];
    char param[32];

    snprintf(url, sizeof(url), BASEURL"login/checkUser");
    snprintf(param, sizeof(param), "_json_att=");

    if(perform_request(url, POST, (void *)param, nxt) < 0) {
	return -1;
    }

    cJSON *status, *data, *flag, *root;

    root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }

    status = cJSON_GetObjectItem(root, "status");
    if(cJSON_IsTrue(status)) {
	data = cJSON_GetObjectItem(root, "data");
	if(!cJSON_IsNull(data)) {
	    flag = cJSON_GetObjectItem(data, "flag");
	    if(cJSON_IsTrue(flag)) {
		cJSON_Delete(root);
		return 0;
	    }
	}
    }
    cJSON_Delete(root);
    return 1;
}

int init_my12306()
{
    char url[128];
    char param[64];

    //snprintf(url, sizeof(url), BASEURL"passport?redirect=/otn/login/userLogin");
    //perform_request(url, GET, NULL, nxt);
    snprintf(url, sizeof(url), "https://kyfw.12306.cn/passport/web/auth/uamtk");
    perform_request(url, POST, (void *)"appid=otn", nxt);

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
	}
    } else {
	cJSON_Delete(root);
	return 1;
    }
    snprintf(url, sizeof(url), "%s", BASEURL"index/initMy12306");
    perform_request(url, GET, NULL, nxt);

    cJSON_Delete(root);
    return 0;
}

int get_passenger_tickets_for_submit(cJSON *root)
{
    cJSON *limitSeatTicketDTO = cJSON_GetObjectItem(root, "limitBuySeatTicketDTO");
    if(cJSON_IsNull(limitSeatTicketDTO)) {
	return 1;
    }

    cJSON *seat_type_codes = cJSON_GetObjectItem(limitSeatTicketDTO, "seat_type_codes");
    if(cJSON_IsNull(seat_type_codes)) {
	return 1;
    }
    int i, j;
    int array_size = cJSON_GetArraySize(seat_type_codes);
    cJSON *st[32];
    tinfo.seat_type[0] = 0;

    for(i = 0; i < array_size; i++) {
	cJSON *seat_obj = cJSON_GetArrayItem(seat_type_codes, i);
	if(cJSON_IsNull(seat_obj)) {
	    return 1;
	}
	cJSON *id = cJSON_GetObjectItem(seat_obj, "id");
	if(!cJSON_IsString(id)) {
	    return 1;
	}
	st[i] = id;
    }

    st[i] = NULL;
    i = 0;

    if(config._prefer_seat_type[0][0] == 0) {
	strncpy(tinfo.seat_type, st[0]->valuestring, sizeof(tinfo.seat_type));
    } else {
	while(config._prefer_seat_type[i][0]) {
	    j = 0;
	    while(st[j]) {
		if(strcmp(config._prefer_seat_type[i], st[j]->valuestring) == 0) {
		    strncpy(tinfo.seat_type, st[j]->valuestring, sizeof(tinfo.seat_type));
		    break;
		}
		j++;
	    }
	    if(st[j]) {
		break;
	    }
	    i++;
	}
    }

    if(tinfo.seat_type[0] == 0 && 
	    strstr(config._prefer_seat_type_all, "5") != NULL && 
	    cJSON_IsString(st[0])) {
	strncpy(tinfo.seat_type, st[0]->valuestring, sizeof(tinfo.seat_type));
    } else if(tinfo.seat_type[0] == 0) {
	return 2;
    }
    char *pi = tinfo.passenger_tickets;
    i = 0;
    while(cur_passenger[i].code[0]) {
	if(i) {
	    *pi = '_';
	    pi++;
	}
    	snprintf(pi, sizeof(tinfo.passenger_tickets) - (pi - tinfo.passenger_tickets), "%s,0,1,%s,1,%s,%s,N", 
		tinfo.seat_type, cur_passenger[i].passenger_name, 
	    	cur_passenger[i].passenger_id_no, cur_passenger[i].mobile_no);
	pi += strlen(pi);
	i++;
    }
    return 0;
}

int get_old_passenger_for_submit()
{
    char *pi = tinfo.old_passenger;
    int i = 0;
    while(cur_passenger[i].code[0]) {
	snprintf(pi, sizeof(tinfo.old_passenger) - (pi - tinfo.old_passenger), "%s,1,%s,1_", 
		cur_passenger[i].passenger_name, cur_passenger[i].passenger_id_no);
	pi += strlen(pi);
	i++;
    }
    return 0;
}

bool set_cur_passenger()
{
    int i = 0, j, k = 0;
    while(config._passenger_name[i][0]) {
	j = 0;
	while(pinfo[j].code[0]) {
	    if(strcmp(pinfo[j].passenger_name, config._passenger_name[i]) == 0) {
		memcpy(cur_passenger + k, pinfo + j, sizeof(struct passenger_info));
		k++;
	    }
	    j++;
	}
	i++;
    }
    memset(cur_passenger + k, 0, sizeof(struct passenger_info));
    return cur_passenger[0].code[0] ? true : false;
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
    char date_format[16];
    strftime(date_format, sizeof(date_format), "%Y-%m-%d", ts);

    snprintf(url, sizeof(url), BASEURL"leftTicket/submitOrderRequest");
    snprintf(param, sizeof(param), "secretStr=%s&train_date=%c%c%c%c-%c%c-%c%c&back_train_date=%s&tour_flag=dc&purpose_codes=ADULT&query_from_station_name=%s&query_to_station_name=%s&undefined", t_info->secretStr, 
	    *p_stdate, *(p_stdate + 1), *(p_stdate + 2), *(p_stdate + 3), *(p_stdate + 4), *(p_stdate + 5), *(p_stdate + 6), *(p_stdate + 7),
	    date_format, config._from_station_name, config._to_station_name);

    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if(cJSON_IsTrue(status)) {
	cJSON_Delete(root);
	return 0;
    } else {
	cJSON *msg_array = cJSON_GetObjectItem(root, "messages");
	if(!cJSON_IsNull(msg_array)) {
	    int asize = cJSON_GetArraySize(msg_array);
	    int i;
	    for(i = 0; i < asize; i++) {
		cJSON *msg = cJSON_GetArrayItem(msg_array, i);
		if(cJSON_IsString(msg)) {
		    printf("error: %s\n", msg->valuestring);
		    if(strstr(msg->valuestring, "您还有未处理的订单")) {
			cJSON_Delete(root);
			do_cleanup();
			exit(5);
		    }
		}
	    }
	}
    }
    cJSON_Delete(root);
    return 1;
}


int check_order_info(cJSON *json)
{
    char url[64];
    char param[512];

    snprintf(url, sizeof(url), BASEURL"confirmPassenger/checkOrderInfo");
    
    if(get_passenger_tickets_for_submit(json) == 2) {
	printf("No such seat type: %s\n", config._prefer_seat_type_all);
	return 3;
    }

    char *url_encode_passenger = curl_easy_escape(curl, tinfo.passenger_tickets, strlen(tinfo.passenger_tickets));
    get_old_passenger_for_submit();
    char *url_encode_old_passenger = curl_easy_escape(curl, tinfo.old_passenger, strlen(tinfo.old_passenger));
    snprintf(param, sizeof(param), "cancel_flag=2&bed_level_order_num=000000000000000000000000000000&passengerTicketStr=%s&"
	    "oldPassengerStr=%s&tour_flag=dc&randCode=&whatsSelect=1&_json_att=&REPEAT_SUBMIT_TOKEN=%s", 
	    url_encode_passenger, url_encode_old_passenger, tinfo.repeat_submit_token);

    curl_free(url_encode_passenger);
    curl_free(url_encode_old_passenger);

    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }

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
		goto failed;
	    }
	    cJSON *ifShowPassCode = cJSON_GetObjectItem(data, "ifShowPassCode");
	    if(!cJSON_IsString(ifShowPassCode)) {
		goto failed;
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
failed:
    cJSON_Delete(root);
    return 1;
}

int get_left_ticket_str(cJSON *root)
{
    cJSON *left, *key, *train_location, *queryLeftTicketRequestDTO, *ypInfoDetail;
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
    queryLeftTicketRequestDTO = cJSON_GetObjectItem(root, "queryLeftTicketRequestDTO");
    if(cJSON_IsNull(queryLeftTicketRequestDTO)) {
	return 1;
    }
    ypInfoDetail = cJSON_GetObjectItem(queryLeftTicketRequestDTO, "ypInfoDetail");
    if(!cJSON_IsString(ypInfoDetail)) {
	return 1;
    }
    strncpy(tinfo.yp_info_detail, ypInfoDetail->valuestring, sizeof(tinfo.yp_info_detail));
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
    int time_zone, tz_hh = 0, tz_mm = 0;
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
    size_t bf_len = strlen(buffer);
    char *pbf = buffer + bf_len;
    if(snprintf(pbf, sizeof(buffer) - bf_len, " GMT%s%02d%02d (CST)", tz_hh > 0 ? "+" : "-", tz_hh, tz_mm) <= 0) {
	return -1;
    }
    bf_len = strlen(buffer);
    size_t i, j;
    for(i = 0, j = 0; i < bf_len; i++, j++) {
	if(j >= size) {
	    break;
	}
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
    
    snprintf(url, sizeof(url), BASEURL"confirmPassenger/getQueueCount");
    snprintf(param, sizeof(param), "train_date=%s&train_no=%s&stationTrainCode=%s&seatType=%s&fromStationTelecode=%s&"
	    "toStationTelecode=%s&leftTicket=%s&purpose_codes=00&train_location=%s&_json_att=&REPEAT_SUBMIT_TOKEN=%s", 
	    date_format, t_info->train_no, 
	    t_info->station_train_code, tinfo.seat_type, 
	    t_info->from_station_telecode, t_info->to_station_telecode, 
	    tinfo.yp_info_detail, tinfo.train_location, 
	    tinfo.repeat_submit_token);
    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if(!cJSON_IsTrue(status)) {
	cJSON *msg_arr = cJSON_GetObjectItem(root, "messages");
	if(!cJSON_IsNull(msg_arr)) {
	    cJSON *msg = cJSON_GetArrayItem(msg_arr, 0);
	    if(cJSON_IsString(msg)) {
		printf("%s\n", msg->valuestring);
	    }
	}
	cJSON *url = cJSON_GetObjectItem(root, "url");
	if(!cJSON_IsNull(url) && strcmp(url->valuestring, "/leftTicket/init") == 0) {
	    cJSON_Delete(root);
	    return 7;
	}
	cJSON_Delete(root);
	return 8;
    }
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if(cJSON_IsNull(data)) {
	cJSON_Delete(root);
	return 1;
    }
    cJSON *ticket_str = cJSON_GetObjectItem(data, "ticket");
    if(!cJSON_IsString(ticket_str)) {
	cJSON_Delete(root);
	return 1;
    }
    printf("当前余票：%s, ", ticket_str->valuestring);
    long left_ticket = strtol(ticket_str->valuestring, NULL, 10);
    if(left_ticket < config._passenger_count) {
	printf("余票不足，将%s加入黑名单%d秒\n", 
		t_info->station_train_code, config._block_time);
	add_train_to_black_list(t_info);
	cJSON_Delete(root);
	return 5;
    }
    if(strcmp(ticket_str->valuestring, "0") == 0) {
	printf("该数据为缓存，将%s加入黑名单%d秒\n", 
		t_info->station_train_code, config._block_time);
	add_train_to_black_list(t_info);
	cJSON_Delete(root);
	return 6;
    }
    cJSON *countT_str = cJSON_GetObjectItem(data, "countT");
    if(!cJSON_IsString(countT_str)) {
	cJSON_Delete(root);
	return 1;
    }
    printf("当前排队人数：%s\n", countT_str->valuestring);
    int cur_queue_count = (int)strtol(countT_str->valuestring, NULL, 10);
    if(cur_queue_count > config._max_queue_count) {
	cJSON_Delete(root);
	return 3;
    }
    cJSON *op2_str = cJSON_GetObjectItem(data, "op_2");
    if(!cJSON_IsString(op2_str)) {
	cJSON_Delete(root);
	return 4;
    }
    if(strcmp(op2_str->valuestring, "true") == 0) {
	printf("当前排队人数超过余票张数，该数据为缓存，将%s加入黑名单%d秒\n", 
		t_info->station_train_code, config._block_time);
	add_train_to_black_list(t_info);
	cJSON_Delete(root);
	return 2;
    }

    cJSON_Delete(root);
    return 0;
}

int confirm_single_queue(struct train_info *t_info)
{
    char url[128];
    char param[1024];
    char choose_seats[64];
    int i = 0;
    memset(choose_seats, 0, sizeof(choose_seats));

    char *cs = choose_seats;
    while(config._choose_seats[i][0]) {
	snprintf(cs, sizeof(choose_seats) - (cs - choose_seats), "%s",
		config._choose_seats[i]);
	cs += strlen(cs);
	i++;
    }

    snprintf(url, sizeof(url), BASEURL"confirmPassenger/confirmSingleForQueue");

    char *url_encode_passenger = curl_easy_escape(curl, tinfo.passenger_tickets, strlen(tinfo.passenger_tickets));
    char *url_encode_old_passenger = curl_easy_escape(curl, tinfo.old_passenger, strlen(tinfo.old_passenger));

    snprintf(param, sizeof(param), "passengerTicketStr=%s&oldPassengerStr=%s&randCode=&purpose_codes=00&key_check_isChange=%s&"
	    "leftTicketStr=%s&train_location=%s&choose_seats=%s&seatDetailType=000&whatsSelect=1&roomType=00&dwAll=N&"
	    "_json_att=&REPEAT_SUBMIT_TOKEN=%s", 
	    url_encode_passenger, url_encode_old_passenger,
	    tinfo.key_is_change, tinfo.left_ticket, tinfo.train_location,
	    choose_seats,
	    tinfo.repeat_submit_token);
    curl_free(url_encode_passenger);
    curl_free(url_encode_old_passenger);

    if(perform_request(url, POST, param, nxt) < 0) {
	return -1;
    }
    cJSON *root = cJSON_Parse(chunk.memory);
    if(cJSON_IsNull(root)) {
	return 1;
    }
    cJSON *status = cJSON_GetObjectItem(root, "status");
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if(!cJSON_IsTrue(status)) {
	if(cJSON_IsString(data)) {
	    printf("出票失败，原因：%s，该数据为缓存，", data->valuestring);
	}
	printf("订单确认失败，将%s加入黑名单%d秒\n", t_info->station_train_code, config._block_time);
	add_train_to_black_list(t_info);
	cJSON_Delete(root);
	return 1;
    }
    if(cJSON_IsNull(data)) {
	cJSON_Delete(root);
	return 1;
    }
    cJSON *submit_status = cJSON_GetObjectItem(data, "submitStatus");
    if(!cJSON_IsTrue(submit_status)) {
	cJSON *err_msg = cJSON_GetObjectItem(data, "errMsg");
	if(cJSON_IsString(data)) {
	    printf("出票失败，原因：%s\n，该数据为缓存，", err_msg->valuestring);
	}
	printf("订单确认失败，将%s加入黑名单%d秒\n", t_info->station_train_code, config._block_time);
	add_train_to_black_list(t_info);
	cJSON_Delete(root);
	return 1;
    }
    cJSON_Delete(root);
    return 0;
}

int query_order_wait_time()
{
    char url[256];
    char time_num[16];
    time_t tvnow;

    time(&tvnow);
    srand(tvnow);
    snprintf(time_num, sizeof(time_num), "%ld%d", tvnow, rand() % 1000);

    snprintf(url, sizeof(url), "%s?random=%s&tourFlag=dc&_json_att=&REPEAT_SUBMIT_TOKEN=%s", 
	    BASEURL"confirmPassenger/queryOrderWaitTime", time_num, tinfo.repeat_submit_token); 

    if(perform_request(url, GET, NULL, nxt) < 0) {
	return -1;
    }
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
	cJSON_Delete(root);
	return 2;
    }
    cJSON *waitTime = cJSON_GetObjectItem(data, "waitTime");
    if(!cJSON_IsNumber(waitTime)) {
	cJSON_Delete(root);
	return 1;
    }
    cJSON *order_id = cJSON_GetObjectItem(data, "orderId");
    if(cJSON_IsString(order_id)) {
	strncpy(tinfo.order_no, order_id->valuestring, sizeof(tinfo.order_no));
    }
    if(waitTime->valueint == -1 || waitTime->valueint == -100) {
	cJSON_Delete(root);
	return 0;
    }
    if(waitTime->valueint == -2) {
	cJSON *msg = cJSON_GetObjectItem(data, "msg");
	if(cJSON_IsString(msg)) {
	    printf("error: %s\n", msg->valuestring);
	}
    }
    cJSON_Delete(root);
    return waitTime->valueint;
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
    printf("当前车次：%s，乘车日期：%s，出发时间：%s，到达时间：%s，出发站：%s，到达站：%s\n", t_info->station_train_code,
	    t_info->start_train_date, t_info->start_time, t_info->arrive_time, t_info->from_station_name, t_info->to_station_name);
    printf("开始提交订单请求...\n");
    if(check_user() != 0) {
	printf("当前用户未登陆，开始登陆...\n");
	if(user_login() != 0) {
	    return 1;
	}
    }
    printf("正在预提交订单请求...\n");
    if(start_submit_order_request(ptrain) == 0) {
	printf("正在获取预提交令牌环...\n");
	passenger_initdc();
	cJSON *root = cJSON_Parse(tinfo.ticketInfoForPassengerForm);
	if(cJSON_IsNull(root)) {
	    return 1;
	}
	if(get_left_ticket_str(root) != 0) {
	    printf("获取预提交令牌环失败\n");
	    cJSON_Delete(root);
	    return 1;
	}
	printf("正在获取乘车人信息...\n");
	get_passenger_dtos(tinfo.repeat_submit_token);
	if(!set_cur_passenger()) {
	    int i = 0;
	    while(config._passenger_name[i][0]) {
		printf("No such passenger: %s\n", config._passenger_name[i]);
		i++;
	    }
	    printf("Available passsengers are list below:\n");
	    i = 0;
	    while(pinfo[i].code[0]) {
		printf("%s\n", pinfo[i].passenger_name);
		i++;
	    }
	    do_cleanup();
	    exit(3);
	}

	printf("正在检查预提交订单...\n");
	int ret = check_order_info(root);
	if(ret == 0) {
	    printf("当前无需验证码，继续提交订单...\n");
	} else if(ret == 2) {
	    printf("当前需要验证码，等待验证...\n");
	    if(show_varification_code(1) != 0) {
		cJSON_Delete(root);
		return 1;
	    }
	} else if(ret == 3) {
	    printf("将%s加入黑名单%d秒\n", t_info->station_train_code, config._block_time);
	    add_train_to_black_list(t_info);
	    cJSON_Delete(root);
	    return 1;
	} else {
	    cJSON_Delete(root);
	    return 1;
	}
	printf("正在获取排队人数...\n");
	if((ret = get_queue_count(root, ptrain)) != 0) {
	    cJSON_Delete(root);
	    return ret;
	}
	printf("正在确认订单请求...\n");
	if(confirm_single_queue(ptrain) != 0) {
	    cJSON_Delete(root);
	    return 1;
	}
	int wait_time;
	printf("系统出票中，开始等待...\n");
	while((wait_time = query_order_wait_time()) != 0) {
	    if(wait_time == -2) {
		cJSON_Delete(root);
		do_cleanup();
		exit(4);
	    }
	    printf("继续等待系统出票...\n");
	    sleep(3);
	}
	if(wait_time != 0) {
	    cJSON_Delete(root);
	    return 1;
	}
	printf("系统出票成功，请在30分钟内登陆kyfw.12306.cn支付订单\n");
	if(config._mail_username[0] != 0 && config._mail_password[0] != 0 && config._mail_server[0] != 0) {
	    if(sendmail(&config, ptrain->station_train_code, ptrain->from_station_name, ptrain->to_station_name,
			ptrain->start_train_date, ptrain->start_time) == CURLE_OK) {
		printf("邮件已发送\n");
	    } else {
		printf("发送邮件失败\n");
	    }
	}
	//result_order_for_dc_queue(tinfo.order_no);
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

void print_help()
{
    printf("Usage: ./tickethelper [OPTION]\n");
    printf("Available options are list below:\n");
    printf("  -c, --config\tspecify configuration file.\n");
    printf("  -h, --help\tprint this help message and exit.\n");
    printf("  -q, --queit\tqueit mode, don't output each train information.\n");
    printf("  -Q, --query-only\tquery only mode, only query ticket information and disable auto order ticket function.\n");
    printf("  -v, --verbose\toutput more datail information about connection and debug information.\n");
    printf("  -V, --version\tprint application version and exit.\n");
    exit(0);
}
