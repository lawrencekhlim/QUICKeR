#include "ue_interface.h" 
#include "utils.h"

#include <arpa/inet.h>
#include <unistd.h>
 
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>


pthread_rwlock_t data_lock;
pthread_mutex_t time_lock;

double getlocktime = 0.0;
double putlocktime = 0.0;
double preplocktime = 0.0;
double updlock1time = 0.0;
double updlock2time = 0.0;
double totaledupdtime = 0.0;
double totalupdtime = 0.0;
double updtime = 0.0;
double totalgettime = 0.0;
double gettime = 0.0;
double totalputtime = 0.0;
double puttime = 0.0;
double totalpreptime = 0.0;
double preptime = 0.0;


#define MAX 12000000 // = 10 MB
#define SA struct sockaddr


int multi_mode = 0;
// multi_mode = 0 means no locking
// multi_mode = 1 means locking, and do not unlock during updates
// multi_mode = 2 means locking, and unlock during updates.

char* ue_scheme_name;
int PORT = 7000;

int backup_plaintext_size = 100;

static void *myThreadFun(void* t_num) {
	ue* ue_scheme = updatable_encryption_function(ue_scheme_name); 	
	int thread_num = *((int*)(t_num));

	int database_size = 0;
	char* value_array = (char *) malloc(MAX); 
	int data_size = 0;

	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else {
		printf("Socket successfully created..\n");
	}
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT + thread_num);


	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else {
		printf("Socket successfully binded..\n");
	}

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed on port %d...\n", PORT + thread_num);
		exit(0);
	}
	else {
		printf("Server listening on port %d..\n", PORT + thread_num);
	}
	len = sizeof(cli);

	char* buff = (char*)malloc (MAX);
	char* buff2 = (char*)malloc (MAX);
	// ----------- Where the loop starts ------------

	int prevconnection = -1;
	double start, end, start2, a, b, c;

	char* temp = (char*) malloc (15);
	
	for(;;) {

		double tmpgetlocktime = 0.0;
		double tmpputlocktime = 0.0;
		double tmppreplocktime = 0.0;
		double tmpupdlock1time = 0.0;
		double tmpupdlock2time = 0.0;
		double tmptotalupdtime = 0.0;
		double tmptotaledupdtime = 0.0;
		double tmpupdtime = 0.0;
		double tmptotalgettime = 0.0;
		double tmpgettime = 0.0;
		double tmptotalputtime = 0.0;
		double tmpputtime = 0.0;
		double tmptotalpreptime = 0.0;
		double tmppreptime = 0.0;


		start = get_time_in_seconds();
		connfd = accept(sockfd, (SA*)&cli, (socklen_t*)&len);
		if (connfd < 0) {
			printf("server accept failed...\n");
		}
		else {
			//printf("server accept the client...\n");
		}
	        end = get_time_in_seconds();
	        double connect_to_server = (end - start);
		

		bzero (buff2, MAX);
		int recvlen = recvall(connfd, buff, MAX);

		

		start2 = get_time_in_seconds();

		// parse into commands
		int number_of_splits;
		char** command_splits = split_by_space(buff, &number_of_splits, recvlen);
		char* command = command_splits[0];

		// Run that command (set,get,updatable_update)
		int len = 0;
		char* ok_ret = "OK";

		if (strlen(command) == 3 && strncmp(command, "GET", 3) == 0) {
			a = get_time_in_seconds();
			if (multi_mode != 0) {
				pthread_rwlock_rdlock (&data_lock);
			}
			b = get_time_in_seconds();
			tmpgetlocktime += b - a;
			start = get_time_in_seconds();

			sprintf (buff2, "RESULT %d ", data_size);
			int i = strlen (buff2);
			memcpy ((void*) (buff2 + i), value_array, data_size);
			len = i + data_size;
            end = get_time_in_seconds();
			tmpgettime += end - start;
			tmptotalgettime += end - start2;		
		}
		else if (strlen (command) == 3 && strncmp(command, "PUT", 3) == 0) {
			a = get_time_in_seconds();
			if (multi_mode != 0) {
				pthread_rwlock_wrlock(&data_lock);
			}
			b = get_time_in_seconds();
			tmpputlocktime += b - a;
			start = get_time_in_seconds();
			
			char* num_bytes = command_splits[1];
			data_size = atoi(num_bytes);
			memcpy (value_array, (void*) buff + strlen (command_splits[0]) + 2 + strlen (command_splits[1]), data_size);	
			memcpy (buff2, ok_ret, 2); 
			len = 2;
            end = get_time_in_seconds();
			tmpputtime += end - start;
			tmptotalputtime += end - start2;
		}
		else if (strlen (command) == 4 && strncmp (command, "PREP", 4) == 0) {
			a = get_time_in_seconds();
			if (multi_mode != 0) {
				pthread_rwlock_rdlock (&data_lock);
			}
			b = get_time_in_seconds();
			tmppreplocktime += b - a;
			start = get_time_in_seconds();
			
			// what is the start and end byte that the server should return
			char* start_byte = command_splits[1];
			int sb = atoi (start_byte);
			char* end_byte = command_splits[2];
			int eb = atoi (end_byte);

			if (sb == 0 && eb == 0) {
				eb = data_size;
			}
            
			sprintf (buff2, "RESULT %d ", eb - sb);
			int i = strlen (buff2);
			
			// copy the bytes requested to send back.
			memcpy ((void*)(buff2 + i), (void*) (value_array + sb), eb-sb);
			len = i + eb - sb;

            end = get_time_in_seconds();
			tmppreptime += end - start;
			tmptotalpreptime += end - start2;
		}
		else if (strlen (command) == 3 && strncmp (command, "UPD", 3) == 0) {
			a = get_time_in_seconds();
			if (multi_mode == 1) {
				pthread_rwlock_wrlock (&data_lock);
			}
			else if (multi_mode == 2) {
				pthread_rwlock_rdlock (&data_lock);
			}
			b = get_time_in_seconds();
			tmpupdlock1time += b - a;
			start = get_time_in_seconds();

			// how many bytes is the token
			char* num_bytes = command_splits[1];
			int i  = atoi(num_bytes);
			
			// setup token parameter
			struct token t;
			t.len = i;
			t.tokptr = (void*)(buff + strlen (command_splits[0]) + 2 + strlen (command_splits[1])); 	
		

			// setup ciphertext parameter
			struct ctxt c;
			c.len = data_size;
			c.ctxtptr = value_array;
			
			if (multi_mode == 2) {
				pthread_rwlock_unlock(&data_lock);
			}
			double start4 = get_time_in_seconds();
			struct ctxt c2;
			ue_scheme->upd(&c2, &t, &c);	

			double start3 = get_time_in_seconds();
			tmpupdtime += start3 - start4;

			if (multi_mode == 2)  {
				pthread_rwlock_wrlock(&data_lock);
			}

			double end2 = get_time_in_seconds();
			tmpupdlock2time += end2 - start3;
			
			// copy over data to value_array
			memcpy (value_array, c2.ctxtptr, c2.len);
			free (c2.ctxtptr);

			memcpy (buff2, ok_ret, 2);
			len = 2;
            end = get_time_in_seconds();
			tmptotalupdtime += end - start;
			tmptotaledupdtime += end - start2;
		}
		if (multi_mode != 0) {
			pthread_rwlock_unlock(&data_lock);
		}

		buff2[len+1] = 0;
		sendall(connfd, buff2, len+1);

		buff2[0] = 0;
		
		close(connfd);

		free(command_splits[0]);
		free(command_splits);

		pthread_mutex_lock(&time_lock);
		getlocktime += tmpgetlocktime;
		putlocktime += tmpputlocktime;
		preplocktime += tmppreplocktime;
		updlock1time += tmpupdlock1time;
		updlock2time += tmpupdlock2time;
		totaledupdtime += tmptotaledupdtime;
		totalupdtime += tmptotalupdtime;
		updtime += tmpupdtime;
		totalgettime += tmptotalgettime;
		gettime += tmpgettime;
		totalputtime += tmptotalputtime;
		puttime += tmpputtime;
		totalpreptime += tmptotalpreptime;
		preptime += tmppreptime;

		pthread_mutex_unlock(&time_lock);

	}
}



// Driver function
int main(int argc, char *argv[])
{

    	if (argc < 5) {
		printf ("Usage: ./server <ue scheme> <num_threads> <portnum> <multithreaded>\n");
		exit (0);
    	}

    	pthread_rwlock_init(&data_lock, NULL);
    	pthread_mutex_init(&time_lock, NULL);

	ue_scheme_name = argv[1];
	char* tmpptr;
	int num_threads = strtol(argv[2], &tmpptr, 10);
	PORT = strtol(argv[3], &tmpptr, 10);
	multi_mode = strtol (argv[4], &tmpptr, 10);

	for (int i = 0; i < num_threads; i++) {
		pthread_t thread_id;
		int* thread_num = (int*) malloc(sizeof(int));
		*thread_num = i;
		pthread_create(&thread_id, NULL, &myThreadFun, (void*) thread_num);
	}
	
	// wait forever
	for(;;) {
		sleep (20);
		printf ("times are:\n");
		printf ("updlock1time\t= %f\n", updlock1time);
		printf ("updlock2time\t= %f\n", updlock2time);
		printf ("updtime \t= %f\n", updtime);
		printf ("totalupdtime\t= %f\n", totalupdtime);	
		printf ("totaledupdtime\t= %f\n", totaledupdtime);
		printf ("getlocktime \t= %f\n", getlocktime);
		printf ("gettime \t= %f\n", gettime);
		printf ("totalgettime \t= %f\n", totalgettime);
		printf ("putlocktime \t= %f\n", putlocktime);
		printf ("puttime \t= %f\n", puttime);
		printf ("totalputtime \t= %f\n", totalputtime);
		printf ("preplocktime\t= %f\n", preplocktime);
		printf ("preptime\t= %f\n", preptime);
		printf ("totalpreptime\t= %f\n", totalpreptime);
	}
}

