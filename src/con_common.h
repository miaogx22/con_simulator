/*
 * Copyright (C) miaogx@aliyun.com
 */

#ifndef CON_SIMULATOR_CON_COMMON_H
#define CON_SIMULATOR_CON_COMMON_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define IP_STR_SIZE             32
#define IP_PORT_SIZE            64
#define READWRITE_BUFFER_SIZE   1024
#define READ_CONTENT_SIZE       1024
#define MAX_LOCAL_IP_LIST       100             //最大IP：PORT 行数支持


#define VERSION                "0.0.1"
#define TITLE_INFO_SIZE         128
#define INPUT_FILE_SIZE         512

#define CON_TYPE_CLIENT         1
#define CON_TYPE_SERVER         2

#define CON_ECHO_NO             0
#define CON_ECHO_YES            1
#define CON_CONTENT_YES         2

#define CON_STATUS_INIT         0
#define CON_STATUS_CONNECTING   1
#define CON_STATUS_FAILED       2
#define CON_STATUS_CONNECTED    3
#define CON_STATUS_EXIT         4
#define CON_STATUS_LISTINGED    5
#define CON_STATUS_ACCEPT       6

int con_status;
/*
 * IP和PORT地址对, 由-f指定的文件来获取
 */
struct ip_port_pair_s{
    char client_ip[IP_STR_SIZE];
    char server_ip[IP_STR_SIZE];
    int port;
};

struct con_ip_port_s {
    struct ip_port_pair_s ip_port[IP_STR_SIZE][MAX_LOCAL_IP_LIST];;
    int num;
};

/*
 * 标记连接与传输信息
 */
struct con_info_s {
    char title_info[TITLE_INFO_SIZE];
    int total_con;                      /*当前已经建立的并发数*/
    int closed_con;                     /*被对端关闭了的并发数*/
    int error_con;                      /*收到-1的并发数*/
    int connect_con;                    /*仍处于正常连接状态的并发数*/
    size_t recv_size;                   /*总接收的字节数*/
    size_t send_size;                   /*总收到的字节数*/
};

void con_info_init(struct con_info_s *con_info, char *info_str);

void ouch(int sig);
int setnonblocking(int fd);
char *read_data(FILE *fp, char *buf);
int read_ip_port_list(struct con_ip_port_s *ip_port, char *input, int type);
int read_content_from_file(char *input_file, char *content);
int simulator_read_write(int cliendfd, char *rwbuffer, char *content, struct con_info_s *con_info, int flag);
void *ui_thread_run(void *arg);
#endif //CON_SIMULATOR_CON_COMMON_H
