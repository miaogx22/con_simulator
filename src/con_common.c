/*
 * Copyright (C) miaogx@aliyun.com
 */
#include "con_common.h"

void ouch(int sig)
{
    con_status = CON_STATUS_EXIT;
    printf("con_simulator client exit.\n");
    exit(0);
}



int setnonblocking(int fd)
{
    if( fcntl(fd, F_SETFL, fcntl( fd, F_GETFD, 0 )|O_NONBLOCK ) == -1 ){
        printf("Set blocking failed.\n");
        return -1;
    }
    return 0;
}

char *read_data(FILE *fp, char *buf)
{
    return fgets(buf, IP_PORT_SIZE, fp);
}


/*
 * 返回IP：PORT组合的行数
 * flag: 1: client   2: server
 */
int read_ip_port_list(struct con_ip_port_s *ip_port, char *input, int type)
{
    if(access(input, R_OK) == -1){
        perror("Not have ip list file.");
        return -1;
    }

    int i;
    char *p;
    FILE *fp;
    char buf[IP_PORT_SIZE];

    fp = fopen(input, "r");
    if(fp== NULL){
        perror("Open ip list file.");
        return -1;
    }

    for(i = 0; i<MAX_LOCAL_IP_LIST; i++) {
        memset(buf, 0, IP_PORT_SIZE);
        p = read_data(fp, buf);
        if (!p) {
            break;
        }

        char *sp = buf;
        int flag = 0;
        while ((p = strsep(&sp, ":")) != NULL) {
            if(type == CON_TYPE_CLIENT){
                if(flag == 0){
                    strcpy(ip_port->ip_port[i]->client_ip, p);
                }

                if(flag == 1){
                    strcpy(ip_port->ip_port[i]->server_ip, p);
                }

                if(flag == 2){
                    ip_port->ip_port[i]->port = atoi(p);
                }
            } else if(type == CON_TYPE_SERVER){
                if(flag == 0){
                    strcpy(ip_port->ip_port[i]->server_ip, p);
                }

                if(flag == 1){
                    ip_port->ip_port[i]->port = atoi(p);
                }
            }

            flag++;
        } //while
    } //for

    ip_port->num = i;

    return ip_port->num;
}

void con_info_init(struct con_info_s *con_info, char *info_str)
{
    memset(con_info, 0, sizeof(struct con_info_s));
    strcpy(con_info->title_info, info_str);
}


/*
 * flag:
 * 0: 不进行回写
 * 1: 进行数据回显
 * 2: 由content发生数据
 */
int simulator_read_write(int cliendfd, char *rwbuffer, char *content, struct con_info_s *con_info, int flag)
{
    int ret = 0;

    memset(rwbuffer, 0, READWRITE_BUFFER_SIZE);
    ret = recv(cliendfd, rwbuffer, READWRITE_BUFFER_SIZE - 1, 0);
    if (ret > 0){
        con_info->recv_size += ret;
        if(flag == CON_ECHO_YES){
            ret = write(cliendfd, rwbuffer, ret);
            if(ret > 0){
                con_info->send_size += ret;
            }
        } else if(flag == CON_CONTENT_YES){
            ret = write(cliendfd, content, READ_CONTENT_SIZE);
            if(ret > 0){
                con_info->send_size += ret;
            }
        } else {
            ;
        }
    } else if (ret < 0){
        con_info->error_con++;
        con_info->connect_con--;
        return -2;
    } else {
        con_info->closed_con++;
        con_info->connect_con--;
        return -1;
    }

    return 0;
}

/*
 * 客户端与服务端都使用的当前状态显示线程
 */
void *ui_thread_run(void *arg)
{
    time_t seconds;
    struct tm *p;
    struct con_info_s *con_info = arg;

    while(con_status != CON_STATUS_EXIT){
        sleep(3);
        time(&seconds);
        p = localtime(&seconds);
        printf("server_simulator: %04d-%02d-%02d %02d:%02d:%02d  total_con:%d  closed:%d  error_con:%d  connect_con:%d  recv:%ld  send:%ld\n",
               p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
               con_info->total_con, con_info->closed_con, con_info->error_con, con_info->connect_con, con_info->recv_size, con_info->send_size);
    }

}

/*
 * 读取内容文件
 */
int read_content_from_file(char *input_file, char *content)
{
    int ret, fd;

    if(access(input_file, R_OK) == -1){
        perror("Not have content file.");
        return -1;
    }

    fd = open(input_file, O_RDONLY);

    ret = read(fd, content, READ_CONTENT_SIZE);
    close(fd);

    return ret;

}