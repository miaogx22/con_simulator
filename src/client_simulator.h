/*
 * Copyright (C) miaogx@aliyun.com
 */

#ifndef CON_SIMULATOR_CLIENT_SIMULATOR_H
#define CON_SIMULATOR_CLIENT_SIMULATOR_H

#include "con_common.h"

char text[] =                  "1234567890";
/*
 * 标识一个TCP会话
 */
struct con_session_s {
    int socket_fd;
    struct sockaddr_in local_address;    //绑定的地址结构
    int server_port;                      //服务器的端口
    socklen_t client_len;
    char *server_ip;
    char *local_ip;
};


struct con_connect_s {
    char input_file[INPUT_FILE_SIZE];
    char content_file[INPUT_FILE_SIZE];
    struct con_ip_port_s *ip_port;
    int sleep_time;
    int send_time;

    int unit;                              /*共有多少个测试单元*/
    int unit_num;                          /*每一个测试单元的并发数*/
    int totoal_num;                        /*所有测试单元的总并发, 计划的*/

    struct con_info_s *con_info;
    struct con_session_s **con_session;

    int epollfd;
    struct epoll_event ev;
    struct epoll_event *events;

    char *rwbuffer;
    char *content;
    int content_size;
};


#endif //CON_SIMULATOR_CLIENT_SIMULATOR_H
