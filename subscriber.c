// SERBOI FLOREA-DAN 325CB
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file) {
	fprintf(stderr, "Usage: %s <ID_Client> <IP_Server> <Port_Server>\n", file);
	exit(0);
}

// se extrag parametri dintr-un sir
void extract_parameters(char operation[153], char parameters[3][51], int *n) {
	char parameter[51];
	int k;
	unsigned int i;
	k = -1, *n = -1;
	for (i = 0; i <= 2; i++)
		strcpy(parameters[i], "");
	for (i = 0; i <= strlen(operation); i++)
		if (operation[i] != ' ' && operation[i] != '\0' 
			&& operation[i] != '\n') {
			k++;
			parameter[k] = operation[i];
		} else {
			parameter[k + 1] = '\0';
			if (strlen(parameter) > 0)
			{
				(*n)++;
				if((*n) >= 3 && strcmp(parameters[0], "subscribe") == 0){
					(*n)++;
					return;
				}
				if((*n) >= 2 && strcmp(parameters[0], "unsubscribe") == 0){
					(*n)++;
					return;
				}
				strcpy(parameters[*n], parameter);
			}
			k = -1;
			strcpy(parameter, "");
		}
	(*n)++;
}

void print_info(info_msg* new_msg){		
	printf("%s:%hu - ", new_msg->ip, new_msg->port);
	fflush(stdout);
	udp_msg* udp_m = (udp_msg *)new_msg->topic;
	printf("%s - ", udp_m->topic);
	fflush(stdout);
	char id = udp_m->identifier;
	switch (id) {
		case 0: ;
			printf("INT - ");
			fflush(stdout);
			uint8_t sign = udp_m->un.int_t.sign;
			uint32_t module = ntohl(udp_m->un.int_t.module);
			int number;
			if(sign == 1){
				number = -module;
			} else {
				number = module;
			}
			printf("%d\n", number);
			fflush(stdout);
			break;
		case 1: ;
			printf("SHORT_REAL - ");
			fflush(stdout);
			uint16_t short_real_t = ntohs(udp_m->un.short_real_t);
			double f = short_real_t * 1.0 / 100;
			printf("%.2f\n", f);
			fflush(stdout);
			break;
		case 2: ;
			printf("FLOAT - ");
			fflush(stdout);
			sign = udp_m->un.float_t.sign;
			module = ntohl(udp_m->un.float_t.module);
			uint8_t neg_power_module = udp_m->un.float_t.neg_power_module;
			double d = module;
			while(neg_power_module){
				d = d / 10;
				neg_power_module--;
			}
			if(sign == 1){
				d = - d;
			}
			printf("%f\n", d);
			fflush(stdout);
			break;

		case 3: ;
			printf("STRING - ");
			fflush(stdout);
			printf("%s\n", udp_m->un.string_t);
			fflush(stdout);
			break;
	}
}

int main(int argc, char *argv[])
{
	if (argc < 4) {
		usage(argv[0]);
	}
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[LEN];   // buffer
	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	// se goleste multimea de descriptori de citire (read_fds)
	// si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// id-ul unui client are maxim 10 caractere ASCII
	char client_ID[10];
	strcpy(client_ID, argv[1]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	
	// trimit mesaj cu ID CLIENT catre server imediat dupa ce m-am conectat la server
	tcp_id_msg id_msg;
	memset(&id_msg, 0, sizeof(tcp_id_msg));
	id_msg.length = sizeof(tcp_id_msg);
	strcpy(id_msg.client_ID, client_ID);
	n = send(sockfd, &id_msg, sizeof(tcp_id_msg), 0);
	DIE(n == -1, "send ID CLIENT");

	// se adauga fd pentru citire in multimea read_fds
	// se adauga noul file descriptor (socketul pe care se primesc mesaje) in multimea read_fds
	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	// Urmatoarele variabile le folosim pentru a trata mesajele primite
	// TCP poate uni mesajele initiale sau le poate imparti in bucati
	// De aceea, vom trata fiecare situatie posibila pentru a reusi
	// sa obtinem mesajul initial

	// Protocolul creat de mine are in structura sa un prim camp care reprezinta lungimea mesajului

	// length_bytes reprezinta numarul de bytes primiti care semnifica lungimea mesajului initial
	uint32_t length_bytes = 0;
	// no_copied_bytes reprezinta numarul de bytes care au fost copiati din mesajul initial
	uint32_t no_copied_bytes = 0;
	// in aux_msg construim mesajul initial
	info_msg aux_msg;
	memset(&aux_msg, 0, LEN);
	// aux_msg_pos reprezinta un pointer la zona din mesaj la care ne aflam la momentul actual
	char* aux_msg_pos = (char *) &aux_msg;

	while (1) {
		tmp_fds = read_fds; 
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(0, &tmp_fds)) {		
			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			if (strncmp(buffer, "exit", 4) == 0) {
				// inchidere client
				break;
			}
			int no_par;
			char par[3][51];
			extract_parameters(buffer, par, & no_par);
			if(strcmp(par[0], "subscribe") != 0 && strcmp(par[0], "unsubscribe") != 0){
				fprintf(stderr, "Typo for first parameter!\n");
				continue;
			}
			if(atoi(par[2]) != 0 && atoi(par[2]) != 1){
				fprintf(stderr, "SF parameter different than 0 or 1!\n");
				continue;
			}
			if((no_par != 3 && strcmp(par[0], "subscribe") == 0)
			|| (no_par != 2 && strcmp(par[0], "unsubscribe") == 0)) {
				fprintf(stderr, "You entered the wrong number of parameters!\n");
				continue;
			}
			if(strcmp(par[0],"subscribe") == 0) {
				// se trimite mesaj la server cu subscribe topic SF
				tcp_subs_msg subs_msg;
				memset(&subs_msg, 0, sizeof(tcp_subs_msg));
				subs_msg.length = sizeof(tcp_subs_msg);
				strcpy(subs_msg.subs, par[0]);
				strcpy(subs_msg.topic, par[1]);
				subs_msg.SF = par[2][0] - 48;
				n = send(sockfd, &subs_msg, sizeof(tcp_subs_msg), 0);
				DIE(n == -1, "send");
				printf("subscribed %s\n", par[1]);
				fflush(stdout);
			}
			if(strcmp(par[0],"unsubscribe") == 0) {
				// se trimite mesaj la server cu unsubscribe topic
				tcp_subs_msg unsubs_msg;
				memset(&unsubs_msg, 0, sizeof(tcp_subs_msg));
				unsubs_msg.length = sizeof(tcp_subs_msg);
				strcpy(unsubs_msg.subs, par[0]);
				strcpy(unsubs_msg.topic, par[1]);
				n = send(sockfd, &unsubs_msg, sizeof(tcp_subs_msg), 0);
				DIE(n == -1, "send");
				printf("unsubscribed %s\n", par[1]);
				fflush(stdout);
			}
		}
		if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, LEN);
			// receptionez mesaj de la server
			int r = recv(sockfd, buffer, LEN, 0);
			if(r < 0){
				perror("recv error in client");
			}
			uint32_t msg_length = *(uint32_t *) buffer;
			// daca lungimea e 0, inseamna ca e un mesaj care solicita inchiderea clientului
			if(msg_length == 0){
				exit(0);
			}
			else {
				// length reprezinta numarul de bytes ramasi de tratat din mesajul primit
				uint32_t length = r;
				// curr_pos reprezinta un pointer la byte-ul curent pe care il tratam din secventa primita
				char* curr_pos = (char *) buffer;

				while(length > 0){
					if(length_bytes < 4){
						// daca putem face rost de lungime
						if(length >= 4 - length_bytes){
							memcpy(aux_msg_pos, curr_pos, 4 - length_bytes);
							aux_msg_pos += 4 - length_bytes;
							curr_pos += 4 - length_bytes;
							no_copied_bytes += 4 - length_bytes;
							length -= 4 - length_bytes;
							length_bytes = 4;
						} else {
							memcpy(aux_msg_pos, curr_pos, length);
							aux_msg_pos += length;
							curr_pos += length;
							no_copied_bytes += length;
							length_bytes += length;							
							length = 0;
						}
					}
					else {
						// daca avem toata informatia necesara ca sa trimitem mesajul
						if(aux_msg.length - no_copied_bytes <= length){
							memcpy(aux_msg_pos, curr_pos, aux_msg.length - no_copied_bytes);
							aux_msg_pos += aux_msg.length - no_copied_bytes;
							curr_pos += aux_msg.length - no_copied_bytes;
							length -= aux_msg.length - no_copied_bytes;
							no_copied_bytes += aux_msg.length - no_copied_bytes;
							// afisam, acum avem toate informatiile
							print_info(&aux_msg);
							// reinitializam valorile pentru urmatorul mesaj
							length_bytes = 0;
							no_copied_bytes = 0;
							memset(&aux_msg, 0, LEN);
							aux_msg_pos = (char *) &aux_msg;
						}
						else {
							memcpy(aux_msg_pos, curr_pos, length);
							aux_msg_pos += length;
							curr_pos += length;
							no_copied_bytes += length;							
							length = 0;
						}
					}
				}
				memset(buffer, 0, LEN);	
			}
		}
	}
	close(sockfd);
	return 0;
}