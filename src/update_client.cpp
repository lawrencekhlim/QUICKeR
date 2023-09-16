// From update client's perspectice
// Loop:
// 0) Create new key
// 1) download from database if necessary
//  - call dl_for_tokgen () <- returns which bytes to download
//  - send pairs of start and end bytes to database to download bytes from database

//  - set info_ptr to downloaded bytes
// PROBLEM: info_ptr may need information such as number of re-encrypts

// 2) Create the token
//  - tok_gen (ret_token, key1, key2, info_ptr)
// 3) Send token to database
//


// From database's perspective
// Loop:
// On receive "download" command,
// Send back bytes according to pairs of start and end bytes
// On receive "update" command,
// call upd(ret_ctxt, token, old_ctxt)
//


#include "utils.h"
#include "ue_interface.h" 

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>


#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#define MAX 12000000
//#define MAX 10000
#define SA struct sockaddr

int timer_expired = 0;
int num_ops = 0;
pthread_mutex_t lock;


char* ue_scheme_name;
int ptxt_size = 100;
char* plaintext;
char* server_ip_addr;
int PORT = 7000;

int backup_plaintext_size = 100;

void secureget(int sockfd, ue* ue_scheme, struct key* cryptokey)
{
	char* buff = (char*)malloc(MAX);
	int n;

	bzero(buff, MAX);
	n = 0;

	char* input = "GET";
	strncat(buff, input, strlen (input));

	sendall(sockfd, buff, strlen(buff)+1);

	bzero(buff, MAX);
	int recvlen = recvall(sockfd, buff, MAX);

	// parse server output
	int arraylen = 0;
	char ** split_vals = split_by_space (buff, &arraylen, recvlen);
	int ctxt_len = atoi(split_vals[1]);
	int ctxt_start_index = strlen (split_vals[0]) + strlen (split_vals[1]) + 2;
	
	struct ctxt to_decrypt;
	to_decrypt.len = ctxt_len;
	to_decrypt.ctxtptr = buff + ctxt_start_index;

	struct ptxt pltxt;
	ue_scheme->dec (&pltxt, cryptokey, &to_decrypt);
	
	free (pltxt.ptxtptr);
	free (split_vals[0]);
	free (split_vals);
	free (buff);
}

void secureput (int sockfd, ue* ue_scheme, struct key* cryptokey) {
	char* buff = (char*)malloc (MAX);
	bzero(buff, MAX);

	struct ptxt pltxt;
	pltxt.len = ptxt_size;
	pltxt.ptxtptr = (void*) plaintext;
	
	struct ctxt encrypted;
	ue_scheme->enc (&encrypted, cryptokey, &pltxt);

	sprintf(buff, "PUT %d ", encrypted.len);
	int stringlength = strlen (buff);
	memcpy (buff + stringlength, encrypted.ctxtptr, encrypted.len);

	sendall (sockfd, buff, stringlength + encrypted.len);
	
	free (encrypted.ctxtptr);

	bzero (buff, MAX);
	recvall (sockfd, buff, MAX);

	printf ("From Server: %s\n", buff); 
	free (buff);
}

void setup_update(int sockfd, struct prep_tokgen_download* ptd, struct tokgen_download* ret) {
	char* buff = (char*) malloc (MAX);
	bzero (buff, MAX);

	
	if (ptd->end_index == -1 || ptd->start_index == -1) {
		sprintf (buff, "PREP %d %d", 0, 0);
	}
	else if (ptd->end_index - ptd->start_index > 0) {
		sprintf (buff, "PREP %d %d", ptd->start_index, ptd->end_index);
	}
	else {
		// don't send anything if it's unnecessary
		return;
	}
	
	sendall (sockfd, buff, strlen(buff));
	bzero (buff, MAX);
	int recvlen = recvall (sockfd, buff, MAX);
	
	int arraylen = 0;
	char ** split_vals = split_by_space (buff, &arraylen, recvlen);
	int dl_len = atoi(split_vals[1]);
	int dl_start_index = strlen (split_vals[0]) + strlen (split_vals[1]) + 2;
	
	ret->len = dl_len;
	ret->downloadptr = (void*)malloc (dl_len);
	// may need to free this point in perform_update()
	
	memcpy (ret->downloadptr, (void*) (buff + dl_start_index), dl_len);
	free (split_vals[0]);
	free (split_vals);
	free (buff);
}

void perform_update (int sockfd, ue* ue_scheme, struct key* old_key, struct key* new_key, struct tokgen_download* downloaded) {
	char* buff = (char*)malloc (MAX);
	bzero (buff, MAX);
	
	// First create the token
	struct token tk;
	ue_scheme->tokgen (&tk, old_key, new_key, downloaded);

	// Send token to server
	sprintf (buff, "UPD %d ", tk.len);
	int tmp = strlen(buff);
	memcpy (buff + tmp, tk.tokptr, tk.len);
	sendall (sockfd, buff, tmp + tk.len);

	free (tk.tokptr);

	bzero (buff, MAX);
	int recvlen = recvall (sockfd, buff, MAX);
	free (buff);	
}

int connect_to_server(int port) {
	int sockfd;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else {
		//printf("Socket successfully created..\n");
	}
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(server_ip_addr);
	servaddr.sin_port = htons(port);


	// connect the client socket to server socket
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed on port %d...\n", port);
		exit(0);
	}
	else {
		//printf("connected to the server on port %d..\n", port);
	}
	return sockfd;
}

static void* myThreadFun(void* t_num) {
	ue* ue_scheme = updatable_encryption_function (ue_scheme_name); 

	int thread_num = *(int*)(t_num);
	int thread_port = PORT + thread_num;

	printf("Thread %d connecting to port %d\n", thread_num, thread_port);
	//int sockfd = connect_to_server(thread_port);
	
	struct key cryptokey;
	ue_scheme->keygen (&cryptokey);

	int sockfd = connect_to_server(thread_port);
	secureput(sockfd, ue_scheme, &cryptokey);
	close(sockfd);
	int counter = 0;
	while (timer_expired == 0) {
		ue_scheme->increment_num_updates();
		int cur_num_updates = ue_scheme->get_num_updates();
		struct prep_tokgen_download ptd;
        ue_scheme->prepare_tokgen (&ptd);
		
		struct tokgen_download td;	
		// only download if needed
		if (ptd.end_index - ptd.start_index > 0 || (ptd.end_index == -1 || ptd.start_index == -1)) {
			int sockfd = connect_to_server(thread_port);
			setup_update (sockfd, &ptd, &td);
			close(sockfd);
		}

		struct key new_cryptokey;
		ue_scheme->keygen (&new_cryptokey);
		sockfd = connect_to_server (thread_port);
		perform_update (sockfd, ue_scheme, &cryptokey, &new_cryptokey, &td); 	
		close(sockfd);

		free (cryptokey.keyptr);
		
		if (ptd.end_index - ptd.start_index > 0 || (ptd.end_index == -1 || ptd.start_index == -1)) {
			free (td.downloadptr);
		}
		cryptokey = new_cryptokey;

		counter += 1;
		if (counter % 100 == 0) {
			printf ("%d\n", counter);
		}
	}
	printf("Thread %d Has completed\n", thread_num);

	pthread_mutex_lock(&lock);
	num_ops += counter;
	pthread_mutex_unlock(&lock);
	
	free (cryptokey.keyptr);
}

// Driver function
int main(int argc, char** argv) {

    	if (argc < 6) {
		printf ("Usage: ./update_client <ue scheme> <ptxt size> <num_threads> <server_ip_addr> <server_port>\n");
		exit (0);
    	}

	ue_scheme_name = argv[1];

	char* tmpptr;
	ptxt_size = strtol(argv[2], &tmpptr, 10);
	plaintext = (char*) malloc (ptxt_size);
	
	int num_threads = strtol(argv[3], &tmpptr, 10);
	
	server_ip_addr = argv[4];
	PORT = strtol (argv[5], &tmpptr, 10);

	pthread_t* tid = (pthread_t*) malloc (num_threads * sizeof (pthread_t));

    	if (pthread_mutex_init(&lock, NULL) != 0) {
        	printf("\n mutex init has failed\n");
        	return 1;
    	}

	for (int i = 0; i < num_threads; i++) {
		int* thread_num = (int*) malloc(sizeof(int));
		*thread_num = i;
		pthread_create(&(tid[i]), NULL, &myThreadFun, (void*) thread_num);
	}
	
	sleep (200);
	timer_expired = 1;
	for (int i = 0; i < num_threads; i++) {
		pthread_join(tid[i], NULL);
	}
	pthread_mutex_destroy(&lock);
	printf ("ops: %d\n", num_ops);
	printf ("num_rounds: %d\n", 1);

	return 0;
}
