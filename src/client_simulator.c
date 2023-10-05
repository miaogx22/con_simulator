/*
 * Copyright (C) miaogx@aliyun.com
 */

#include "client_simulator.h"



/*
 * 获取命令行参数
 */
int cmd_opt(int argc, char *argv[], struct con_connect_s *connect)
{
    int opt;

    while ((opt = getopt(argc, argv, ":vhc:f:m:s:i:")) != -1) {
        switch(opt){
            case ':':
                printf("option needs a value. you can -h to study. \n");
                return -1;
            case 'v':
                printf("client_simulator:  version: %s\n", VERSION);
                return -1;
            case 'h':
                printf("client_simulator usage: \n \tclient_simulator -f <ip_port_file> -c <concurrent number> -m <msleep> -s <second> -i<input file>\n");
                return -1;
            case 'c':
                connect->unit_num = atoi(optarg);
                break;
            case 'm':
                connect->sleep_time = atoi(optarg);
                break;
            case 's':
                connect->send_time = atoi(optarg);
                break;
            case 'f':
                strcpy(connect->input_file, optarg);
                break;
            case 'i':
                strcpy(connect->content_file, optarg);
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

void con_connect_init(struct con_connect_s *con_connect)
{
    con_connect->con_info = malloc(sizeof(struct con_info_s));
    memset(con_connect->con_info, 0, sizeof(struct con_info_s));
    strcpy(con_connect->con_info->title_info, "client_simulator: ");

    con_connect->con_info->recv_size = 0;
    con_connect->con_info->send_size = 0;

    con_connect->rwbuffer = malloc(READWRITE_BUFFER_SIZE);
    if (con_connect->rwbuffer == NULL){
        perror("Malloc for events failed.\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * 在每一个并发单元中建立TCP连接
 * 注意：此时的session是每一个并发单元里面的session
 */
int run(struct con_session_s *session, struct con_connect_s *con_connect, int index)
{
    int i;
    int len;
    int result;

    struct sockaddr_in address;

    for(i = 0; i < con_connect->unit_num; i++){
        con_connect->con_info->total_con+=1;

        //如果创建socket失败,则直接退出程序
        session[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (session[i].socket_fd == -1){
            perror("create socket:");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(session[i].server_ip);
        address.sin_port = htons(session[i].server_port);
        len = sizeof(address);

        if(strcmp(session[i].local_ip, "0.0.0.0") != 0){
            session[i].local_address.sin_family = AF_INET;
            session[i].local_address.sin_addr.s_addr = inet_addr(session[i].local_ip);
            session[i].client_len = sizeof(session[i].local_address);

            //客户端地址绑定，如果bind失败,则忽略当前连接,直接进入下一次连接
            bind(session[i].socket_fd, (struct sockaddr *)&session[i].local_address, session[i].client_len);
            if(result == -1) {
                printf("bind %s failed. sys:%s \n", session[i].local_ip, strerror (errno));
                con_connect->con_info->error_con += 1;
                close(session[i].socket_fd);
                usleep(100);
                continue;
            }
        }

        //如果连接失败, 则忽略当前连接,直接进入下一次连接动作
        result = connect(session[i].socket_fd, (struct sockaddr *)&address, len);
        if(result == -1) {
            printf("connect %s:%d failed. exit. sys:%s \n",session[i].server_ip, session[i].server_port, strerror (errno));
            con_connect->con_info->error_con += 1;
            close(session[i].socket_fd);
            continue;
        }

        /*
         * 添加进入epoll管理
         * 如果有一个epoll失败的话,则直接退出程序
         */
        setnonblocking(session[i].socket_fd);
        con_connect->ev.events = EPOLLIN | EPOLLET;
        con_connect->ev.data.fd = session[i].socket_fd;
        if (epoll_ctl(con_connect->epollfd, EPOLL_CTL_ADD, session[i].socket_fd, &con_connect->ev) == -1) {
            perror("epoll_ctl: conn_sock");
            exit(EXIT_FAILURE);
        }

/* 关闭数据发送测试*/
#if 0
        result = write(socket_fd[i], text, sizeof(text));
        if(result <= 0) {
            perror("write for server failed.\n");
            printf("\nThe test was fialed. last socket index is:%d\n\n", i - 1);
            return -1;
        }
#endif
        con_connect->con_info->connect_con+=1;

        usleep(con_connect->sleep_time *1000);
    }

    return 0;
}


/*
 * 并发建立线程
 */
void *connect_thread_run(void *arg)
{
    int ret;
    int i, j;
    struct con_connect_s *con_connect = arg;

    pthread_detach(pthread_self());

    con_status = CON_STATUS_CONNECTING;

    //遍历所有的测试单元，构建连接
    for(i=0; i < con_connect->unit; i++){
        printf("bind local ip is:%s to build connect.\n", con_connect->ip_port->ip_port[i]->client_ip);

        con_connect->con_session[i] = malloc(sizeof(struct con_session_s) * con_connect->unit_num);
        memset(con_connect->con_session[i], 0, sizeof(struct con_session_s) * con_connect->unit_num);

        //在每一个测试单元中，分别构建地址结构
        for(j = 0; j < con_connect->unit_num; j++){
            con_connect->con_session[i][j].local_ip = con_connect->ip_port->ip_port[i][0].client_ip;
            con_connect->con_session[i][j].server_ip = con_connect->ip_port->ip_port[i][0].server_ip;
            con_connect->con_session[i][j].server_port = con_connect->ip_port->ip_port[i][0].port;
        }

        //这里执行connect的创建动作
        ret = run(con_connect->con_session[i], con_connect, i);
        if(ret != 0){
            con_status = CON_STATUS_FAILED;
            return NULL;
        }

        printf("The bind ip: %s connect to:%s:%d was successful. and built %d connect  and con total:%d.\n",
               con_connect->ip_port->ip_port[i][0].client_ip,
               con_connect->ip_port->ip_port[i][0].server_ip,
               con_connect->ip_port->ip_port[i][0].port,
               con_connect->unit_num,
               con_connect->con_info->connect_con);
    }

    con_status = CON_STATUS_CONNECTED;
    printf("build all connected end.......................\n");
    return NULL;
}

/*
 * 定时发送数据线程
 */
void *timed_send_thread(void *arg)
{
    int i, j;
    int ret;
    struct con_connect_s *con_connect = arg;

    pthread_detach(pthread_self());

    for(;;){
        if(con_status != CON_STATUS_CONNECTED){
            sleep(1);
            continue;
        }

        if(con_status == CON_STATUS_EXIT){
            break;
        }

        for(i = 0; i < con_connect->unit; i++){
            for(j = 0; j < con_connect->unit_num; j++){
                if(strlen(con_connect->content) > 0){
                    ret = write(con_connect->con_session[i][j].socket_fd, con_connect->content, con_connect->content_size);
                    if(ret > 0){
                        con_connect->con_info->send_size += ret;
                    }
                } else {
                    ret = write(con_connect->con_session[i][j].socket_fd, text, strlen(text));
                    if(ret > 0){
                        con_connect->con_info->send_size += ret;
                    }
                }
            } //for(j)
        }//for(i)

        if(con_connect->send_time != 0){
            sleep(con_connect->send_time);
        } else {
            //默认10S钟发一次数据
            sleep(10);
        }
    }
}

/*
 * main
 *
 */
int main(int argc, char **argv)
{
    con_status = CON_STATUS_INIT;
    //退出信号处理
    struct sigaction act;
    act.sa_handler = ouch;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);

    int ret;
    int n, nfds;
    struct con_connect_s *con_connect;
    pthread_t ui_thread;
    pthread_t connect_thread;

    con_connect = malloc(sizeof(struct con_connect_s));
    con_connect_init(con_connect);

    //获取命令行参数
    ret = cmd_opt(argc, argv, con_connect);
    if (ret != 0){
        exit(-1);
    }

    //由文件读取IP：PORT组合
    if(strlen(con_connect->input_file) > 0){
        con_connect->ip_port = malloc(sizeof(struct con_ip_port_s));
        con_connect->unit = read_ip_port_list(con_connect->ip_port, con_connect->input_file, CON_TYPE_CLIENT);
        if(con_connect->unit <= 0){
            exit(1);
        }
    }

    con_connect->totoal_num = con_connect->unit_num * con_connect->unit;             /*计算总的并发数*/
    con_connect->con_session = malloc(sizeof(struct con_session_s) * con_connect->unit_num);
    memset(con_connect->con_session, 0, sizeof(struct con_session_s) * con_connect->unit_num);

    con_connect->events = malloc(sizeof(struct epoll_event) * con_connect->totoal_num);
    if (con_connect->events == NULL){
        perror("Malloc for events failed.\n");
        exit(EXIT_FAILURE);
    }

    memset(con_connect->events, 0, con_connect->totoal_num);

    con_connect->epollfd = epoll_create1(0);
    if (con_connect->epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    con_connect->ev.events = EPOLLIN;

    //由文件读取发生内容
    con_connect->content = malloc(READ_CONTENT_SIZE);
    memset(con_connect->content, 0, READ_CONTENT_SIZE);

    if(strlen(con_connect->content_file) > 0){
        ret = read_content_from_file(con_connect->content_file, con_connect->content);
        if(ret < 0){
            printf("read file failed, exit.\n");
            exit(-1);
        }

        con_connect->content_size = ret;
    } else {
        memcpy(con_connect->content, text, strlen(text));
    }

    /*创建一个显示线程*/
    pthread_create(&ui_thread, NULL, ui_thread_run, con_connect->con_info);

    /*创建一个并发建立线程*/
    pthread_create(&connect_thread, NULL, connect_thread_run, con_connect);

    /*创建一个进行定时发送的线程*/
    pthread_create(&connect_thread, NULL, timed_send_thread, con_connect);

    while (con_status == CON_STATUS_CONNECTING){
        sleep(1);

        if(con_status == CON_STATUS_FAILED){
            exit(1);
        }
    }

    for (;;) {
        nfds = epoll_wait(con_connect->epollfd, con_connect->events, con_connect->totoal_num, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; ++n) {
            ret = simulator_read_write(con_connect->events[n].data.fd, con_connect->rwbuffer, con_connect->content, con_connect->con_info, CON_ECHO_NO);

            if (ret < 0){
                /*从epoll中移除此socket*/
                if (epoll_ctl(con_connect->epollfd, EPOLL_CTL_DEL, con_connect->events[n].data.fd, NULL) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }

                close(con_connect->events[n].data.fd);
            }
        }

        if(con_status == CON_STATUS_EXIT){
            printf("TMP -------\n");
        }
    } /*for(;;)*/

    return 0;

}
