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
#include <random>

#define MAX 12000000
#define SA struct sockaddr



int timer_expired = 0;
int read_ops = 0;
int write_ops = 0;
pthread_mutex_t lock;

char* ue_scheme_name;
int ptxt_size = 100;
char* server_ip_addr;
int PORT = 7000;
char* plaintext;
double read_portion = 0.5;

int seed;

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

	cksum (buff, recvlen);

	// parse server output
	int arraylen = 0;
	char ** split_vals = split_by_space (buff, &arraylen, recvlen);
	int ctxt_len = atoi(split_vals[1]);
	int ctxt_start_index = strlen (split_vals[0]) + strlen (split_vals[1]) + 2;
	
	struct ctxt to_decrypt;
	to_decrypt.len = ctxt_len;
	to_decrypt.ctxtptr = buff + ctxt_start_index;
	cksum ((char*) to_decrypt.ctxtptr, to_decrypt.len);

	struct ptxt pltxt;
	ue_scheme->dec (&pltxt, cryptokey, &to_decrypt);
	free (pltxt.ptxtptr);
	free (split_vals[0]);
	free (split_vals);
	free (buff);
}

void secureput (int sockfd, ue* ue_scheme, struct key* cryptokey) {
	char* buff = (char*) malloc (MAX);
	bzero(buff, MAX);

	struct ptxt pltxt;
	pltxt.len = ptxt_size;
	pltxt.ptxtptr = (void*) plaintext;

	struct ctxt encrypted;
	ue_scheme->enc (&encrypted, cryptokey, &pltxt);

	sprintf(buff, "PUT %d ", encrypted.len);
	int stringlength = strlen (buff);
	memcpy (buff + stringlength, encrypted.ctxtptr, encrypted.len);

	cksum ((char*)encrypted.ctxtptr, encrypted.len);
	sendall (sockfd, buff, stringlength + encrypted.len);
	
	
	free (encrypted.ctxtptr);

	bzero (buff, MAX);
	recvall (sockfd, buff, MAX);
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
	int thread_seed = seed + thread_num;

	printf("Thread %d connecting to port %d\n", thread_num, thread_port);

	srand (time(NULL));

	struct key cryptokey;
	ue_scheme->keygen (&cryptokey);
	int sockfd = connect_to_server(thread_port);
	secureput(sockfd, ue_scheme, &cryptokey);
	close(sockfd);

    // Random dist and rng for selecting puts vs gets operations
    std::mt19937 ops_rng(thread_seed);
    std::discrete_distribution<> ops_dist({read_portion,1-read_portion});

	int read_counter = 0;
	int write_counter = 0;
	while (timer_expired == 0) {
		double random = ops_dist(ops_rng);
		if (random == 1) {
			free (cryptokey.keyptr);
			ue_scheme->keygen (&cryptokey);
			int sockfd = connect_to_server(thread_port);
			secureput(sockfd, ue_scheme, &cryptokey);
			close(sockfd);
			write_counter += 1;
		}
		else {
			sockfd = connect_to_server(thread_port);
			secureget(sockfd, ue_scheme, &cryptokey);
			close(sockfd);
			read_counter += 1;
		}
	}
	printf("Thread %d Has completed\n", thread_num);
	pthread_mutex_lock(&lock);
	read_ops += read_counter;
	write_ops += write_counter;
	pthread_mutex_unlock(&lock);
	free (cryptokey.keyptr);
}

// Driver function
int main(int argc, char** argv)
{
    	if (argc < 8) {
		printf ("Usage: ./client <ue scheme> <ptxt size> <num_threads> <server_ip_addr> <server_port> <read_proportion (float)> <seed>\n");
		exit (0);
    	}
	ue_scheme_name = argv[1];

	char* tmpptr;
    seed = strtol(argv[7], &tmpptr, 10);
	ptxt_size = strtol(argv[2], &tmpptr, 10);
	plaintext = (char*) malloc (ptxt_size);
	
	int num_threads = strtol(argv[3], &tmpptr, 10);
	
	server_ip_addr = argv[4];
	PORT = strtol (argv[5], &tmpptr, 10);
	read_portion = atof (argv[6]);

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
	printf ("read_ops: %d\n", read_ops);
	printf ("write_ops: %d\n", write_ops);

	return 0;
}
