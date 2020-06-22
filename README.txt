Tema este compusa din:
	- server.c reprezinta serverul care asigura legatura intre clientii din platforma, cu scopul publicarii si abonarii la mesaje
	- subscriber.c reprezinta un client TCP se conectează la server, el poate primi (in orice moment) de la tastatură comenzi de tipul subscribe si unsubscribe si afisează pe ecran mesajele primite de la server
	- helpers.h unde sunt definite structurile de date, tipurile si MACRO-urile folosite in implementarea temei
	- glist.h care contine definitia unei liste generice si semnaturile functiilor pentru liste
	- glist.c unde sunt implementate operatii pe liste generice cum ar fi: adaugarea unui element la inceputul listei, cautarea unui element, eliminarea unui element, distrugerea si eliberarea memoriei alocate unei liste

Observatie: Pentru fiecare structura de data definita, am definit si un tip cu aceeasi denumire(ca sa evitam folosirea repetata a cuvantului struct).

Astfel, tipurile de date definite sunt urmatoarele:
	- int_type : contine reprezentarea din cerinta a tipului INT, cu o valoare pe un byte pentru semn si o valoare pe 4 bytes pentru modul
	- float_type : contine reprezentarea din cerinta a tipului FLOAT, cu o valoare pe un byte pentru semn, o valoare pe 4 bytes pentru modul si o valoare pe 1 byte ce reprezintă modulul puterii negative a lui 10 cu care trebuie inmultit modulul pentru a obtine numărul original (in modul)
	- udp_msg : contine reprezentarea datagramelor primite de la clientii UDP
	- info_msg : contine reprezentarea mesajelor cerute de clientii TCP(care sunt abonati la un topic de interes) de la server. Aici avem ip-ul si portul client-ului UDP precum si continutul datagramei trimise de el
	- topic_SF : contine topic-ul si optiunea de Store&Forward
	- tcp_client_ident : contine informatiile de identificare a clientilor TCP conectati, si anume : filedescriptor-ul, portul, ip-ul, ID-ul
	- tcp_client : contine informatiile utile despre un client TCP si anume: ID-ul, daca e sau nu conectat, topicurile de interes, mesajele stocate
	- tcp_id_msg : contine reprezentarea unui mesaj de la clientul TCP prin care se transmite ID-ul sau catre server
	- tcp_subs_msg : contine reprezentarea unui mesaj de subscribe/unsubscribe trimis de catre clientul TCP serverului

Mentiuni:
	- pentru server:
		- avem grija ca nr de bytes trimisi sa fie minim. Astfel nr de bytes trimisi este egal cu lungimile elementelor de baza din structura mesajului pe care il trimitem clientului TCP (elementele din protocolul definit de noi care sunt fixe) + lungimea elementelor variabile(valorile continute) care depinde de tip-ul trimis. Pentru string adaugam strict lungimea mesajului. Astfel, evitam situatiile in care am fi trimis aprope 1500 de bytes pentru aproximativ 70-80 de bytes de informatie utila
		- sunt tratate erori cat mai diverse, unele cu DIE(care afecteaza functionalitatea server-ului), altele prin afisare la stderr
	- pentru client:
		- TCP poate uni mesajele initiale sau le poate imparti in bucati la un flux mare de mesaje in timp scurt. De aceea, este tratata fiecare situatie posibila pentru a reusi sa obtinem mesajul initial.
		- sunt tratate erori cat mai diverse, unele cu DIE(care afecteaza functionalitatea clientului), altele prin afisare la stderr
	- in general : toata memoria alocata este dezalocata in mod corect.
