// SERBOI FLOREA-DAN 325CB
#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include "glist.h"

#define DIE(condition, message) \
	do { \
		if ((condition)) { \
			fprintf(stderr, "[%d]: %s\n", __LINE__, (message)); \
			perror(""); \
			exit(1); \
		} \
	} while (0)

// dimensiunile calupului de date
#define BUFLEN 1551
#define LEN 1574
#define BASELEN 73

// numarul maxim de clienti in asteptare
#define MAX_CLIENTS	100

struct int_type
{
	// 0 - positive; 1 - negative
	uint8_t	sign;
	uint32_t	module;
}__attribute__((packed));

typedef struct int_type int_type;

struct float_type
{
	// 0 - positive; 1 - negative
	uint8_t	sign;
	uint32_t	module;
	uint8_t neg_power_module;
}__attribute__((packed));

typedef struct float_type float_type;

struct udp_msg {
	char topic[50];
	// INT - 0; SHORT_REAL - 1; FLOAT - 2; STRING - 3;
	uint8_t identifier;
	union
	{
		int_type int_t;
		uint16_t	short_real_t; // module * 100
		float_type float_t;
		char string_t[1500];
	} un;
} __attribute__((packed));

typedef struct udp_msg udp_msg;

struct info_msg {
	uint32_t length;
	char ip[16];
	unsigned short port;
	char topic[50];
	// INT - 0; SHORT_REAL - 1; FLOAT - 2; STRING - 3;
	uint8_t identifier;
	union
	{
		int_type int_t;
		uint16_t	short_real_t; // module * 100
		float_type float_t;
		char string_t[1501];
	} un;
} __attribute__((packed));

typedef struct info_msg info_msg;

struct topic_SF{
	char topic[51];
	char SF;
} __attribute__((packed));

typedef struct topic_SF topic_SF;

struct tcp_client_ident{
	int fd;
	unsigned short portno;
	char ip[16];
	char client_ID[10];
} __attribute__((packed));

typedef struct tcp_client_ident tcp_client_ident;

struct tcp_client {
	char client_ID[10];
	char connected;
	list topics;
	list stored_messages;
} __attribute__((packed));

typedef struct tcp_client tcp_client;

struct tcp_id_msg {
	uint32_t length;
	char client_ID[10];
} __attribute__((packed));

typedef struct tcp_id_msg tcp_id_msg;

struct tcp_subs_msg {
	uint32_t length;
	char subs[12];
	char topic[51];
	char SF;
} __attribute__((packed));

typedef struct tcp_subs_msg tcp_subs_msg;

#endif
