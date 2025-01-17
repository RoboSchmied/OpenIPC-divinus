#pragma once

#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "jpeg.h"
#include "mp4/mp4.h"
#include "mp4/nal.h"
#include "region.h"

extern char keepRunning;

int start_server();
int stop_server();

void send_jpeg(unsigned char chn_index, char *buf, ssize_t size);
void send_mjpeg(unsigned char chn_index, char *buf, ssize_t size);
void send_h264_to_client(unsigned char chn_index, const void *p);
void send_mp4_to_client(unsigned char chn_index, const void *p);