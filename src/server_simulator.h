/*
 * Copyright (C) miaogx@aliyun.com
 */


#ifndef CON_SIMULATOR_SERVER_SIMULATOR_H
#define CON_SIMULATOR_SERVER_SIMULATOR_H

#include "con_common.h"

struct con_listen_s {
    int listen_sock;
    socklen_t server_len;
    struct sockaddr_in server_address;
    struct epoll_event ev;
};

struct con_server_s {
    struct con_info_s *con_info;
    struct con_ip_port_s *ip_port;
    struct con_listen_s *listen;
    int listen_num;                     //监听的socket数量
    int unit_num;                       //每个监听单元的并发数
    int total_num;                      //总的并发数
    int echo_flag;                      //是否直接回写客户端数据

    char input_file[INPUT_FILE_SIZE];
    char content_file[INPUT_FILE_SIZE];
    int epollfd;
    struct epoll_event ev;
    struct epoll_event *events;

    char *rwbuffer;
    char *content;
    int content_size;
};

#endif //CON_SIMULATOR_SERVER_SIMULATOR_H
