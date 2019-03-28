/*************************************************************************
	> File Name: common.h
	> Author: 
	> Mail: 
	> Created Time: 2019年03月05日 星期二 18时48分30秒
 ************************************************************************/
#ifndef _COMMON_H
#define _COMMON_H
#include<stdio.h>
#include <arpa/inet.h>
#include<sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <pthread.h>
#include<sys/select.h>
#include<sys/ioctl.h>
#include<math.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<stdbool.h>
#include<zlib.h>
#include<zconf.h>
//#include<mysql/mysql.h>
#endif

#ifdef  DEBUG
#define DBG printf
#else
#define DBG //
#endif

int socket_create(int port);

int socket_udp(int port);

int socket_connect(char *ip, int port);

int check_connect(char *ip, int port, int timeout);

char *my_inet_ntoa(struct in_addr in);

int get_conf_value(char *pathname, char *key_name, char *value);

int write_Pi_log(char *PiMonitorLog, char *format, ...);

