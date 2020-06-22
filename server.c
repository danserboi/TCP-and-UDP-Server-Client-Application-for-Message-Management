// SERBOI FLOREA-DAN 325CB
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <linux/socket.h>
#include "helpers.h"
#include "glist.h"
#include <arpa/inet.h>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <PORT_DORIT>\n", file);
	exit(0);
}

list search_by_client_ID(void *element, list l, int size) {
  list temp = NULL;
  while(l){
    temp = l->next;
    if(memcmp(element,((tcp_client_ident*)l->element)->client_ID, size) == 0)
      return l;
    l = temp; 
  } 
  return NULL;
}

int main(int argc, char *argv[])
{
	int udpsockfd, tcpsockfd, newsockfd, portno;
	char buffer[BUFLEN];// buffer
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	
	// deschidem socket TCP
	tcpsockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcpsockfd < 0, "socket tcp");

	// dezactivam algoritmul Neagle	
	int flags =1;
	setsockopt(tcpsockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(tcpsockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");

	ret = listen(tcpsockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen tcp");

	// se adauga noul file descriptor (socketul tcp pe care se asculta conexiuni) in multimea read_fds
	FD_SET(tcpsockfd, &read_fds);
	// se actualizeaza maximul
	fdmax = tcpsockfd;

	// deschidem socket UDP
	udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udpsockfd < 0, "socket udp");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(udpsockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");

	// se adauga noul file descriptor 
	FD_SET(udpsockfd, &read_fds);
	// se actualizeaza maximul
	if(udpsockfd > tcpsockfd)
		fdmax = udpsockfd;

	// se adauga file descriptor pentru citirea de la tastatura
	FD_SET(0, &read_fds);

	// aceasta lista contine informatiile de identificare a clientilor TCP conectati
	// si anume : filedescriptor-ul, portul, ip-ul, ID-ul
	list tcp_ident_l = NULL;

	// aceasta lista contine informatiile utile ale clientilor TCP(conectati sau deconectati)
	// si anume: ID-ul, daca e sau nu conectat, topicurile de interes, mesajele stocate
	list tcp_clients = NULL;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if(i == 0){
					// se citeste de la tastatura
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					if (strcmp(buffer, "exit\n") == 0) {
						// trebuie sa trimit mesaj clientilor tcp cu exit
						// ca sa se inchida si ei
						list p = tcp_ident_l;
						while(p){
							tcp_client_ident* tcp_client = (tcp_client_ident *) p->element;
							// trimitem un mesaj de exit pe care il codificam cu un int = 0
							uint32_t length = 0;
							n = send(tcp_client->fd, &length, 4, 0);
							DIE(n < 0, "send exit signal");
							p = p->next;
						}
						// inchidere server
						goto closeserver;
					} else {
						fprintf(stderr, "Only exit command is accepted !\n");
					}
				}
				else if (i == tcpsockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(tcpsockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept tcp client");
					
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					// introduc in lista datele stiute la momentul actual despre noul client conectat
					// si anume: filedescriptorul, portul si ip-ul
					tcp_client_ident* tcp_client_id = calloc(1, sizeof(tcp_client_ident));
					tcp_client_id->fd = newsockfd;
					strcpy(tcp_client_id->ip, inet_ntoa(cli_addr.sin_addr));
					tcp_client_id->portno = ntohs(cli_addr.sin_port);
					int c = cons(tcp_client_id, &tcp_ident_l);
					DIE(c == 0, "Failed to allocate !");
				} 
				else if(i == udpsockfd){
					// primim o datagrama UDP
					struct sockaddr_in client_info;
					socklen_t length = sizeof(client_info);
					memset(buffer, 0, BUFLEN);
					int r = recvfrom(i, buffer, BUFLEN, 0, (struct sockaddr*) &client_info, &length);
					DIE(r < 0, "recvfrom UDP");
					info_msg new_msg;
					memset(&new_msg, 0, LEN);						
					strcpy(new_msg.ip, inet_ntoa(client_info.sin_addr));
					new_msg.port = ntohs(client_info.sin_port);
					memcpy(&new_msg.topic, buffer, BUFLEN);
					// avem grija ca nr de bytes trimisi sa fie minim
					// astfel nr de bytes trimisi este egal cu lungimile elementelor
					// de baza din structura(care sunt fixe) 
					// + lungimea elementelor variabile(valoarile continute)
					// care depinde de tip-ul trimis
					// pentru string adaugam doar lungimea mesajului
					if(new_msg.identifier == 0)
						new_msg.length = BASELEN + 5;
					if(new_msg.identifier == 1)
						new_msg.length = BASELEN + 2;
					if(new_msg.identifier == 2)
						new_msg.length = BASELEN + 6;
					if(new_msg.identifier == 3) {
						new_msg.length = BASELEN + strlen(new_msg.un.string_t) + 1;
					}
					// aceasta datagrama trebuie trimisa clientilor abonati la topic si care sunt conectati
					// sau trebuie stocata pentru clientii care sunt abonati dar care sunt deconectati si au SF = 1
					udp_msg* udp_m = (udp_msg *)buffer;
					list p_clients = tcp_clients;
					while(p_clients){
						tcp_client* cl_info = (tcp_client*) p_clients->element;
						list topicL = search(udp_m->topic, cl_info->topics, 50);
						if(topicL != NULL){
							topic_SF* topic = (topic_SF *) topicL->element;
							// identific client-ul conectat
							list client_in_list = search_by_client_ID(cl_info->client_ID, tcp_ident_l, strlen(cl_info->client_ID));					
							if(client_in_list){
								tcp_client_ident* client = (tcp_client_ident*)client_in_list->element;	
								if(topic){
									if(cl_info->connected == 1){
										n = send(client->fd, &new_msg, new_msg.length, 0);
										DIE(n < 0, "send");										
									}
								}
							}
							else {
								if(topic->SF == 1){
									// stochez mesajul pentru client-ul deconectat care are activata optiunea SF
									// pentru topicul curent
									info_msg* store_msg = calloc(1, LEN);
									memcpy(store_msg, &new_msg, LEN);
									int c = cons(store_msg, &cl_info->stored_messages);
									DIE(c == 0, "Failed to allocate !");
								} 
							}
						}
						p_clients = p_clients->next;
					}					
				}
				else {
					// s-au primit date pe unul din socketii de client TCP,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "Recv from TCP Client");
					// daca n este 0, inseamna ca un client TCP s-a deconectat
					if (n == 0) {
						list client_in_list = search(&i, tcp_ident_l, 4);
						// daca client-ul se afla in lista de identificare inseamna ca a fost un client valid
						// adica nu a fost unul duplicat, care a incercat sa intre cu acelasi ID,
						// si caruia i s-a trimis mesaj sa iasa
						if(client_in_list){
							tcp_client_ident* client = (tcp_client_ident*)client_in_list->element;
							// conexiunea s-a inchis
							printf("Client (%s) disconnected.\n", client->client_ID);
							// marchez client-ul ca deconectat in lista cu informatii despre clienti
							list info_client_l = search(client->client_ID, tcp_clients, strlen(client->client_ID));
							if(info_client_l){
								tcp_client* info_client = (tcp_client*) info_client_l->element;
								if(info_client){
									info_client->connected = 0;
									// scot client-ul din lista de idenfifcare a celor conectati
									int el = elim(&i, &tcp_ident_l, strlen(client->client_ID));
									DIE(el == 0, "Failed to remove client from list");
								}
							}
						}
						close(i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);

					} else {
						uint32_t length = *(int *)buffer;
						// verificam daca avem mesaj cu ID-ul client-ului TCP
						// pe care il primim imediat dupa conectare
						// sau este mesaj de tipul subscribe/unsubscribe
						if(length == sizeof(tcp_id_msg)){
							tcp_id_msg* id_msg = (tcp_id_msg*) buffer;
							// cautam client-ul in lista
							list cl = search(id_msg->client_ID, tcp_clients, strlen(id_msg->client_ID));
							// acum ca avem si client ID, avem toate datele pentru identificarea unui client
							// trebuie sa introducem aceasta data in lista cu elementele de identificare ale clientilor TCP
							list client_in_list = search(&i, tcp_ident_l, 4);							
							tcp_client_ident* client = (tcp_client_ident*)client_in_list->element;
							strcpy(client->client_ID, id_msg->client_ID);
							if(cl) {
								tcp_client* tcp_cl = (tcp_client*)cl->element;
								if(tcp_cl->connected == 0){
									// daca exista, a fost deconectat, si are mesaje stocate, trebuie sa ii trimitem toate mesajele 
									printf ("New client (%s) connected from %s:%d.\n", client->client_ID, client->ip, client->portno);
									tcp_cl->connected = 1;
									list mess = tcp_cl->stored_messages;
									while(mess){
										info_msg * message = (info_msg*)mess->element;
										send(i, message, message->length, 0);
										mess = mess->next;
									}
									if(tcp_cl->stored_messages){
										free_all(tcp_cl->stored_messages);
										tcp_cl->stored_messages = NULL;
									}
								} else {
									// daca exista si este conectat, inseamna ca un al doilea client cu acelasi ID a incercat sa se conecteze
									// prin urmare, il vom elimina din lista de identificare
									int el = elim(&i, &tcp_ident_l, 4);
									DIE(el == 0, "2 clients with the same ID.\n");
									// ii trimitem mesaj client-ului ca sa se inchida
									uint32_t length = 0;
									n = send(i, &length, 4, 0);
									DIE(n < 0, "Failed to send exit signal to duplicate client.\n");
								}
							} else {
								// daca nu exista deja, il adaugam in lista
								printf ("New client (%s) connected from %s:%d.\n", client->client_ID, client->ip, client->portno);
								tcp_client* tcp_cl = calloc(1, sizeof(tcp_client));
								strcpy(tcp_cl->client_ID, client->client_ID);
								tcp_cl->connected = 1;
								int c = cons(tcp_cl, &tcp_clients);
								DIE(c == 0, "Failed to allocate !");
							}
						} else {
							// daca e mesaj de tipul subscribe/unsubscribe
							tcp_subs_msg* msg = (tcp_subs_msg*) (buffer);
							list client_in_list = search(&i, tcp_ident_l, 4);							
							tcp_client_ident* client = (tcp_client_ident*)client_in_list->element;
							list cl = search(client->client_ID, tcp_clients, strlen(client->client_ID));
							tcp_client* tcp_cl = (tcp_client*)cl->element;
							if(strcmp(msg->subs, "subscribe") == 0){
								list topics = tcp_cl->topics;
								int ok = 1;
								while(topics){
									char * topic = topics->element;
									if(strcmp(topic, msg->topic) == 0){
										fprintf(stderr, "Already subscribed !\n");
										ok = 0;
									}
									topics = topics->next;
								}
								if(ok == 1){
									// daca nu exista topic-ul in lista, il adaugam(impreuna cu SF)
									topic_SF* top_SF = calloc(sizeof(topic_SF), 1);
									strcpy(top_SF->topic, msg->topic);
									top_SF->SF = msg->SF;
									int c = cons(top_SF, &tcp_cl->topics);
									DIE(c == 0, "Failed to allocate !");
								}
							} else if(strcmp(msg->subs, "unsubscribe") == 0) {
								list topics = tcp_cl->topics;
								int ok = 0;
								while(topics){
									char * topic = topics->element;
									if(strcmp(topic, msg->topic) == 0){
										ok = 1;
										break;
									}
									topics = topics->next;
								}
								if(ok == 0){
									fprintf(stderr, "You cannot unsubscribe from a topic that even doesn't exist in your list !\n");
								} else {
									elim(msg->topic , &tcp_cl->topics, strlen(msg->topic));
								}
							}
							
						}
					}
				}
			}
		}
	}
closeserver: ;
	// eliberare memorie
	list p_clients = tcp_clients;
	while(p_clients){
		tcp_client* cl_info = (tcp_client*) p_clients->element;
		if(cl_info->topics)
			free_all(cl_info->topics);
		if(cl_info->stored_messages)
			free_all(cl_info->stored_messages);
		p_clients = p_clients->next;
	}
	free_all(tcp_clients);
	free_all(tcp_ident_l);
	close(tcpsockfd);
	close(udpsockfd);
	return 0;
}