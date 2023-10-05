/*
 * Copyright (C) miaogx@aliyun.com
 */

#include "con_common.h"
#include "server_simulator.h"


int cmd_opt(int argc, char *argv[], struct con_server_s *server)
{
	int opt;

	while ((opt = getopt(argc, argv, ":vhec:f:i:")) != -1) {
		switch(opt){
			case ':':
				printf("option needs a value. you can -h to study. \n");
				return -1;
			case 'v':
				printf("server_simulator:  version: %s\n", VERSION);
				return -1;
			case 'h':
				printf("server_simulator usage: \n \tserver_simulator -f <ip_port_file> -c <concurrent number>\n");
				return -1;
            case 'c':
                server->unit_num = atoi(optarg);
                break;
		    case 'e':
		        server->echo_flag = CON_ECHO_YES;
		        break;
		    case 'f':
		        strcpy(server->input_file, optarg);
		        break;
            case 'i':
                strcpy(server->content_file, optarg);
                break;
			case '?':
				printf("unknown option: %c\n", optopt);
				return -1;
			default:
				printf("You must use the correct parameter options. you can -h to study.\n");
				return -1;
			}
	}

	return 0;

}

/*
 * 建立所有的监听
 */
int build_listen(struct con_server_s *server)
{
    int i;
    int ret;

    for(i = 0; i < server->listen_num; i++){

        server->listen[i].listen_sock = socket(AF_INET, SOCK_STREAM, 0);

        server->listen[i].server_address.sin_family = AF_INET;
        server->listen[i].server_address.sin_addr.s_addr = inet_addr(server->ip_port->ip_port[i]->server_ip);
        server->listen[i].server_address.sin_port = htons(server->ip_port->ip_port[i]->port);
        server->listen[i].server_len = sizeof(server->listen[i].server_address);

        ret = bind(server->listen[i].listen_sock, (struct sockaddr *)&server->listen[i].server_address, server->listen[i].server_len);
        if(ret == -1){
            close(server->listen[i].listen_sock);
            con_status = CON_STATUS_FAILED;
            printf("bind %s:%d failed. exit. sys:%s \n", server->ip_port->ip_port[i]->server_ip, server->ip_port->ip_port[i]->port,  strerror (errno));
            exit(1);
        }

        if(listen(server->listen[i].listen_sock, 1024) != 0){
            close(server->listen[i].listen_sock);
            printf("listen %s:%d failed. exit. sys:%s \n", server->ip_port->ip_port[i]->server_ip, server->ip_port->ip_port[i]->port,  strerror (errno));
            exit(1);
        }

        //添加进入epoll事件
        server->ev.events = EPOLLIN;
        server->ev.data.fd = server->listen[i].listen_sock;
        if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, server->listen[i].listen_sock, &server->ev) == -1) {
            perror("epoll_ctl: exit.listen_sock");
            exit(EXIT_FAILURE);
        }

        printf("Listen ip:%s:%d unit startup...\n",
                server->ip_port->ip_port[i]->server_ip, server->ip_port->ip_port[i]->port );
    }

    printf("All listening success builded...\n");

    return 0;
}

/*
 * event的accept处理
 */
int is_accept_event(int event_fd, struct con_server_s *server)
{
    int i;
    int ret;
    int conn_sock;
    socklen_t client_len;
    struct sockaddr_in client_address;

    for(i = 0; i < server->listen_num; i++) {
        if (event_fd == server->listen[i].listen_sock) {
            client_len= sizeof(client_address);
            conn_sock = accept(server->listen[i].listen_sock, (struct sockaddr *) &client_address, &client_len);
            if (conn_sock == -1) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            setnonblocking(conn_sock);
            server->listen[i].ev.events = EPOLLIN | EPOLLET;
            server->listen[i].ev.data.fd = conn_sock;

            if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, conn_sock, &server->listen[i].ev) == -1) {
                perror("epoll_ctl: conn_sock");
                exit(EXIT_FAILURE);
            }

            server->con_info->connect_con++;
            server->con_info->total_con++;

            return 1;
        }
    }

    return 0;
}

int server_event_handle(int event_fd, struct con_server_s *server)
{
    int ret;

    //首先判断是不是accept事件
    ret = is_accept_event(event_fd, server);
    if(ret != 0){
        return ret;
    }

    //如果返回0,说明是读写事件
    ret = simulator_read_write(event_fd, server->rwbuffer, server->content, server->con_info, server->echo_flag);
    if (ret < 0){
        //从epoll中移除此socket
        if (epoll_ctl(server->epollfd, EPOLL_CTL_DEL, event_fd, NULL) == -1) {
            perror("epoll_ctl: 2conn_sock");
            exit(EXIT_FAILURE);
        }

        close(event_fd);
    }
}

/*
 * run: ./server_simulator -f ip_list_file -c 10000
 */
int main(int argc, char *argv[])
{
    con_status = CON_STATUS_INIT;
    //信号处理
    struct sigaction act;
    act.sa_handler = ouch;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);

    int ret;
    struct epoll_event *events;
    int n, nfds, epollfd;
    pthread_t ui_thread;
    struct con_server_s *server;
    server = malloc(sizeof(struct con_server_s));
    memset(server, 0, sizeof(struct con_server_s));

    server->echo_flag = CON_ECHO_NO;

	ret = cmd_opt(argc, argv, server);
	if (ret != 0){
		exit(-1);
    }

    server->rwbuffer = malloc(READWRITE_BUFFER_SIZE);

    //由文件读取IP：PORT组合
    if(strlen(server->input_file) > 0){
        server->ip_port = malloc(sizeof(struct con_ip_port_s));
        server->listen_num = read_ip_port_list( server->ip_port, server->input_file, CON_TYPE_SERVER);
        if(server->listen_num <= 0){
            exit(1);
        }
    }

    //由文件读取发生内容
    if(strlen(server->content_file) > 0){
        server->content = malloc(READ_CONTENT_SIZE);
        memset(server->content, 0, READ_CONTENT_SIZE);

        ret = read_content_from_file(server->content_file, server->content);
        if(ret < 0){
            printf("read file failed, exit.\n");
            exit(-1);
        }

        server->content_size = ret;
    }

    server->total_num = server->listen_num * server->unit_num;

    //创建对应数量的events
    events = malloc(sizeof(struct epoll_event) * server->total_num);
    if (events == NULL){
        perror("Malloc for events failed.\n");
        exit(EXIT_FAILURE);
    }
    memset(events, 0, server->total_num);

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    server->epollfd = epollfd;

    server->con_info = malloc(sizeof(struct con_info_s));
    con_info_init(server->con_info, "server_simulator: ");

    /*
     * 建立连接
     */
    server->listen = malloc(sizeof(struct con_listen_s) * server->listen_num);
    build_listen(server);

    //创建一个显示线程
    pthread_create(&ui_thread, NULL, ui_thread_run, server->con_info);

    for (;;) {
        nfds = epoll_wait(epollfd, events, server->total_num, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; ++n) {
            server_event_handle(events[n].data.fd, server);
        }
    } //for(;;)

    return 0;
}
