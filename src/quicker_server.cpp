#include "ue_interface.h" 
#include "utils.h"
#include "hsm_functions/hsm_functions.h"

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
#include <iostream>
#include <fstream>
#include <atomic>


#define MAX 10000000 // = 10 MB
#define message_size 1200000
#define SA struct sockaddr

char* ue_scheme_name;
int PORT = 7000;
int is_multi_upd = 1;
CK_OBJECT_HANDLE current_root_key;
pthread_rwlock_t data_lock;
pthread_rwlock_t root_key_lock;

pthread_mutex_t get_time_lock;
pthread_mutex_t set_time_lock;
pthread_mutex_t split_time_lock;
pthread_mutex_t put_lock;
pthread_mutex_t conflict_locks;
double set_time = 0.0;
double get_time = 0.0;
double split_time = 0.0;
double malloc_time = 0.0;
double retrieve_time = 0.0;
double memcpy_time = 0.0;
double upd_time = 0.0;
double put_time = 0.0;
double put_string_1_time = 0.0;
double put_string_2_time = 0.0;
double parse_string_1_time = 0.0;
int get_conflicts;
int set_conflicts;
				

int* get_tracker;
int* set_tracker;
char** data_store;
int* length_store;


int backup_plaintext_size = 100;

int data_set (char *key, uint8_t *value, int value_length) {
	char* tmpptr;
	int key_index = strtol(key, &tmpptr, 10);
	memcpy(data_store[key_index], value, value_length);
	length_store[key_index] = value_length;
}

uint8_t* data_get (char *key, int *length) {
	char* tmpptr;
	int key_index = strtol(key, &tmpptr, 10);
	*length = length_store[key_index];

	uint8_t* retrieved_value = (uint8_t*)(data_store[key_index]);

	return retrieved_value;
}



static void *myThreadFun(void* t_num) {
	ue* ue_scheme = updatable_encryption_function(ue_scheme_name); 	
	int thread_num = *((int*)(t_num));

	int database_size = 0;
	char value_array[MAX]; 
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
	char* tmpbuff = (char*)malloc (MAX);
	// ----------- Where the loop starts ------------

	int prevconnection = -1;
	double start, end;

	char* temp = (char*) malloc (15);
	
	for(;;) {
		connfd = accept(sockfd, (SA*)&cli, (socklen_t*)&len);
		if (connfd < 0) {
			printf("server accept failed...\n");
		}
		else {
			//printf("server accept the client...\n");
		};
        double connect_to_server = (end - start);


		bzero (buff2, MAX);
		//printf ("Listening\n");
		int recvlen = recvall(connfd, buff, MAX);
		

		// parse into commands
		int number_of_splits;
		char** command_splits = split_by_space(buff, &number_of_splits, recvlen);
		char* command = command_splits[0];


		// Run that command (set,get,updatable_update)
		int len = 0;
		char* ok_ret = "OK";
		
		// Communication protocol:
		// DATA IN REDIS: key:<kv_key>, value: <version_number_len> <root-key-id-len> <wrapped-key-len> <ctxt-len> <version_number> <root-key-id> <wrapped-key> <ctxt> 
		//
		// In:  GET <kv-key-length> <kv-key>
		// Out: RESULT <root-key-id-len> <wrapped-key-len> <ctxt-len> <root-key-id> <wrapped-key> <ctxt> 
		// Out: REDISFAIL 
		//
		// In:  PUT <kv-key-length> <root-key-id-len> <wrapped-key-len> <ctxt-len> <kv-key> <root-key-id> <wrapped-key> <ctxt>
		// Out: OK
		// Out: ROOTKEY <root-key-id-len> <root-key-id>
		//
		// In:  ROOTKEY <root-key-id-len> <root-key-id>
		// Out: OK
		//
		// In:  PREP <kv-key-length> <start-index-len> <end-index-len> <kv-key> <start-index> <end-index>
		// Out: REDISFAIL
		// Out: RESULT <version_number_len> <root-key-id-len> <wrapped-key-len> <download-len> <version_number> <root-key-id> <wrapped-key> <download>
		//
		// In:  UPD <kv-key-length> <version_number_len> <root-key-id-len> <wrapped-key-len> <token-len> <kv-key> <version_number> <root-key-id> <wrapped-key> <token>
		// Out: REDISFAIL
		// Out: OK
		//
		// In:  ROUND <root-key-id-len> <root-key-id>
		// Out: OK
		if (strlen(command) == 3 && strncmp(command, "GET", 3) == 0) {
			int *key_lengths;
			void** key_vals = parse_string (&(buff[4]), 1, &key_lengths);
			char* kv_key = (char*)key_vals[0];
			
			
			char* tmpptr;
			int key_index = strtol(kv_key, &tmpptr, 10);
			
			pthread_rwlock_rdlock(&data_lock);
			//pthread_rwlock_rdlock (&(data_lock[key_index]));
			
			int data_size;
			uint8_t* data = data_get(kv_key, &data_size);
			get_tracker[key_index] += 1;
			
			pthread_rwlock_unlock(&data_lock);

			// If you detect redisfail, then return REDISFAIL
			if (data_size > 0) {
				// call parse string
				int *data_lengths;
				void **data_vals = parse_string((data), 4, &data_lengths);
				CK_OBJECT_HANDLE* root_key_id = (CK_OBJECT_HANDLE*)data_vals[1];
				uint8_t* wrapped_key = (uint8_t*)data_vals[2];
				uint8_t* ctxt = (uint8_t*)data_vals[3];

				// return only the values needed (just put into a uint8_t*)
				int* ret_lengths = new int[3];
				ret_lengths[0] = data_lengths[1];
				ret_lengths[1] = data_lengths[2];
				ret_lengths[2] = data_lengths[3];
				void** ret_vals = (void**) malloc (3 * sizeof(void*));
				ret_vals[0] = (void*) root_key_id;
				ret_vals[1] = (void*) wrapped_key;
				ret_vals[2] = (void*) ctxt;
				memcpy(buff2, "RESULT ", 7);
				int offset = put_string(&(buff2[7]), 3, ret_lengths, ret_vals);
				len = 7 + offset;


				free (ret_lengths);
				free (ret_vals);
				free (data_lengths);
				free (data_vals);

			} else {
				memcpy(buff2, "REDISFAIL", 9);
				len = 9;
			}
			free (key_lengths);
			free (key_vals);
		}
		else if (strlen (command) == 3 && strncmp(command, "PUT", 3) == 0) {
			int * lengths;
			void** vals = parse_string (&(buff[4]), 4, &lengths);
			char* kv_key = (char*)vals[0];
			// root key id
			CK_OBJECT_HANDLE* root_key_id = (CK_OBJECT_HANDLE*)vals[1];
			uint8_t* wrapped_key = (uint8_t*)vals[2];
			uint8_t* ctxt = (uint8_t*)vals[3];
			
			char* tmpptr;
			int key_index = strtol(kv_key, &tmpptr, 10);
			
			pthread_rwlock_wrlock(&data_lock);
			
			pthread_rwlock_rdlock(&root_key_lock);
			bool fail_root_key = (current_root_key != *root_key_id);
			pthread_rwlock_unlock(&root_key_lock);

			if (fail_root_key) {
                                memcpy(buff2, "ROOTKEY ", 8);
                                int* ret_rootkey_lengths = new int[1];
                                ret_rootkey_lengths[0] = 4;
                                void** ret_rootkey_vals = (void**)malloc(1 * sizeof(void*));
                                ret_rootkey_vals[0] = (void*)&current_root_key;
;
                                int offset = put_string(&(buff2[8]), 1, ret_rootkey_lengths, ret_rootkey_vals)


                                len = 8 + offset;

                                free (ret_rootkey_lengths);
                                free (ret_rootkey_vals);
                        }
			else {
				// Check if ctxt is in db. If it is, then use ctxt_version +1. Else, initialize ctxt_version as 0
				
				int data_size;
				uint8_t* data = data_get(kv_key, &data_size);


				//printf ("redis after get in secureput\n");
				int ctxt_version = 0;
				if (data_size > 0) {
					// call parse string
					int *data_lengths;
					void **data_vals = parse_string((data), 4, &data_lengths);
					ctxt_version = *(int*)(data_vals[0]);
					
					free (data_vals);
					free (data_lengths);
					// increment the version number by 1
					ctxt_version++;
				}
				
				// Construct string to put into db
				int* stored_lengths = new int[4];
				stored_lengths[0] = sizeof(int);
				stored_lengths[1] = lengths[1];
				stored_lengths[2] = lengths[2];
				stored_lengths[3] = lengths[3];
				
				void** stored_vals = (void**) malloc (4 * sizeof(void*));
				stored_vals[0] = (void*) &ctxt_version;
				stored_vals[1] = (void*) root_key_id;
				stored_vals[2] = (void*) wrapped_key;
				stored_vals[3] = (void*) ctxt;
				int string_length = put_string(tmpbuff, 4, stored_lengths, stored_vals);
				
				// Set Key
				data_set(kv_key, (uint8_t*) tmpbuff, string_length);

				set_tracker[key_index] += 1;

				// Construct OK return 
				memcpy(buff2, "OK", 2);
				len = 2;

				free(stored_lengths);
				free(stored_vals);
			}
			
			pthread_rwlock_unlock(&data_lock);
			free(vals);
			free(lengths);
		}
		else if (strlen (command) == 4 && strncmp (command, "PREP", 4) == 0) {
			// // what is the start and end byte that the server should return
			// parse input
			int *lengths;
			void** vals = parse_string (&(buff[5]), 3, &lengths);
			char* kv_key = (char*)vals[0];
			int start_index = *(int*)vals[1];
			int end_index = *(int*)vals[2];
			
			char* tmpptr;
			int key_index = strtol(kv_key, &tmpptr, 10);
			
			get_tracker[key_index] = 0;
			set_tracker[key_index] = 0;

			free (lengths);
			free (vals);

			pthread_rwlock_rdlock(&data_lock);
			int data_size;

			uint8_t* data = data_get(kv_key, &data_size);
			pthread_rwlock_unlock(&data_lock);

			if (data_size > 0) {

				int *data_lengths;
				void **data_vals = parse_string((data), 4, &data_lengths);
				int ctxt_version = *(int*)(data_vals[0]);
				CK_OBJECT_HANDLE* root_key_id = (CK_OBJECT_HANDLE*)data_vals[1];
				uint8_t* wrapped_key = (uint8_t*)data_vals[2];
				uint8_t* ctxt;
				int ctxt_len;


				if (end_index == 0) {
					ctxt = (uint8_t*)data_vals[3];
					ctxt_len = data_lengths[3];
				} else {
					ctxt = (uint8_t*)(data_vals[3]) + start_index;
					ctxt_len = end_index - start_index;
				}

				int* ret_lengths = new int[3];
				ret_lengths[0] = 4;
				ret_lengths[1] = data_lengths[1];
				ret_lengths[2] = data_lengths[2];
				ret_lengths[3] = ctxt_len;
				void** ret_vals = (void**) malloc (4 * sizeof(void*));
				ret_vals[0] = (void*) &ctxt_version;
				ret_vals[1] = (void*) root_key_id;
				ret_vals[2] = (void*) wrapped_key;
				ret_vals[3] = (void*) ctxt;
				memcpy(buff2, "RESULT ", 7);
				int offset = put_string(&(buff2[7]), 4, ret_lengths, ret_vals);
				len = 7 + offset;
				
				free (data_lengths);
				free (data_vals);
				free (ret_lengths);
				free (ret_vals);
			} else {
				printf ("Redisfail on prepare\n");
				memcpy(buff2, "REDISFAIL", 9);
				len = 9;
			}

		}

		// UPD <kv-key-length> <version_number_len> <root-key-id-len> <wrapped-key-len> <token-len> <kv-key> <version_number> <root-key-id> <wrapped-key> <token>
		//	<kv-key-length> <root-key-id-len> <wrapped-key-len> <ctxt-len> <kv-key> <root-key-id> <wrapped-key> <ctxt>
		// Out: REDISFAIL
		// Out: OK

		else if (strlen (command) == 3 && strncmp (command, "UPD", 3) == 0) {

			// how many bytes is the token
			int* lengths;
			void** vals = parse_string (&(buff[4]), 5, &lengths);
			char* kv_key = (char*)vals[0];
			int* v_number = (int*)vals[1];

			// root key id
			CK_OBJECT_HANDLE* wrapping_key_ptr = (CK_OBJECT_HANDLE*)vals[2];
			uint8_t* new_wrap = (uint8_t*)vals[3];
			uint8_t* tk = (uint8_t*)vals[4];

			int new_wrap_length = lengths[2];
			int wrapping_key_length = lengths[3];

			// setup token parameter
			struct token t;
			t.len = lengths[4];
			t.tokptr = (void*)tk;	


			bool redisfail = 0;


			char* tmpptr;
			int key_index = strtol(kv_key, &tmpptr, 10);

			// // Get Data from DB
			if (is_multi_upd) {
				pthread_rwlock_rdlock(&data_lock);
			} else {
				pthread_rwlock_wrlock(&data_lock);
			}

			int data_size;
			uint8_t* data = data_get(kv_key, &data_size);

			if (is_multi_upd) {
				pthread_rwlock_unlock(&data_lock);
			}

			redisfail = (data_size <= 0);
			
			// Parse Data from redis
			if (!redisfail) {
				// call parse string
				int *data_lengths;
				void **data_vals = parse_string((data), 4, &data_lengths);
				int ctxt_version  = *(int*)(data_vals[0]);
				CK_OBJECT_HANDLE* root_key_id = (CK_OBJECT_HANDLE*)data_vals[1];
				uint8_t* wrapped_key = (uint8_t*)data_vals[2];
				uint8_t* ctxt = (uint8_t*)data_vals[3];

				// Check ctxtversion here
				if (ctxt_version == *v_number)	{
						
					struct ctxt old_ctxt;
					struct ctxt new_ctxt;
					old_ctxt.len = data_lengths[3];
					old_ctxt.ctxtptr = data_vals[3];
					ue_scheme->upd (&new_ctxt, &t, &old_ctxt); 
					int stored_lengths[4] = {4, lengths[2], lengths[3], new_ctxt.len};
					void* stored_vals [4] = { (void*) &ctxt_version, (void*) wrapping_key_ptr, (void*) new_wrap, (void*) new_ctxt.ctxtptr };
					int string_length = put_string(tmpbuff, 4, stored_lengths, stored_vals);

					if (is_multi_upd) {
						// Do another get to check the ctxt version
						pthread_rwlock_wrlock(&data_lock);
					}
					int data_size2;
					uint8_t* data2 = data_get(kv_key, &data_size2);
						
					redisfail = (data_size2 <= 0);
					
					if (!redisfail) {

						int* data_lengths2;
						void **data_vals2 = parse_string((data2), 4, &data_lengths2);

						//printf("done with parse string\n");
						
						int v_num_after_upd = *(int*) (data_vals2[0]);
						
						//printf("v_num_after_upd: %d\n", v_num_after_upd);
						//printf("*v_number: %d\n", *v_number);
						if (v_num_after_upd == *v_number) {
							// return only the values needed (just put into a uint8_t*)

							// Set Key 
							data_set(kv_key, (uint8_t*)tmpbuff, string_length);

						}
						
						free (data_lengths2);
						free (data_vals2);
					}

					if (is_multi_upd) {
						pthread_rwlock_unlock(&data_lock);
					}
					
					free(new_ctxt.ctxtptr);

				}

				free (data_vals);
				free(data_lengths);
				
			}

			if (!is_multi_upd) {
				pthread_rwlock_unlock(&data_lock);
			}


			int get_temp = get_tracker[key_index];
			int set_temp = set_tracker[key_index];
			if (get_temp > 0 || set_temp > 0) {
				pthread_mutex_lock(&conflict_locks);
				get_conflicts += get_temp;
				set_conflicts += set_temp;
			  	pthread_mutex_unlock(&conflict_locks);
			}
			
			if (redisfail) {
				memcpy(buff2, "REDISFAIL", 9);
				len = 9;
			}
			else {
				// Construct OK return 
				memcpy(buff2, "OK", 2);
				len = 2;
			}

			free (lengths);
			free (vals);
		}
		else if (strlen (command) == 5 && strncmp (command, "ROUND", 5) == 0) {
			int* lengths;
			void** vals = parse_string (&(buff[6]), 1, &lengths);
			CK_OBJECT_HANDLE* new_root_key_id = (CK_OBJECT_HANDLE*)vals[0];
			pthread_rwlock_wrlock(&root_key_lock);
			current_root_key = *new_root_key_id;
			pthread_rwlock_unlock(&root_key_lock);
			
			printf ("current_root_key after ROUND = %d\n", current_root_key);

			memcpy(buff2, "OK", 2);
			len = 2;

			free (lengths);
		}
		
		sendall(connfd, buff2, len);
		buff2[0] = 0;

		close(connfd);

		free(command_splits[0]);
		free(command_splits);
		
	}
}



// Driver function
int main(int argc, char *argv[])
{

    	if (argc < 6) {
		printf ("Usage: ./server <ue scheme> <num_threads> <num_of_messages> <portnum> <is_multithreaded_during_upd>\n");
		exit (0);
    	}

	ue_scheme_name = argv[1];
	char* tmpptr;
	int num_threads = strtol(argv[2], &tmpptr, 10);
	int num_of_messages = strtol(argv[3], &tmpptr, 10);
	PORT = strtol(argv[4], &tmpptr, 10);
	is_multi_upd = strtol (argv[5], &tmpptr, 10);

	if (is_multi_upd == 1 && ue_scheme_name == "naive") {
		printf ("Naive cannot be multi-threaded!\n");
		exit(0);
	}

	pthread_rwlock_init(&data_lock, NULL);
	data_store = (char**)malloc(sizeof(char*) * (num_of_messages));
	for (int i=0; i < (num_of_messages); i++) {
		data_store[i] = (char*)malloc(sizeof(char) * message_size);
	}
	pthread_rwlock_init(&root_key_lock, NULL);

	get_tracker = (int*)malloc(sizeof(int) * (num_of_messages));
	for (int i=0; i < (num_of_messages); i++) {
		get_tracker[i] = 0;
	}

	set_tracker = (int*)malloc(sizeof(int) * (num_of_messages));
	for (int i=0; i < (num_of_messages); i++) {
		set_tracker[i] = 0;
	}

	length_store = (int*)malloc(sizeof(int) * num_of_messages);
	bzero(length_store, sizeof(int) * num_of_messages);

	

	for (int i = 0; i < num_threads; i++) {
		pthread_t thread_id;
		int* thread_num = (int*) malloc(sizeof(int));
		*thread_num = i;
		pthread_create(&thread_id, NULL, &myThreadFun, (void*) thread_num);
	}
	
	// wait forever
	for(;;) {
		printf ("heartbeat\n");
		printf("get conflicts: %d\n", get_conflicts);
		printf("set conflicts: %d\n", set_conflicts);

		std::ofstream heartbeat_output("../scripts/server_heartbeat.txt");
		heartbeat_output << "get conflicts: " << get_conflicts << "\n";
		heartbeat_output << "set conflicts: " << set_conflicts << "\n";
		heartbeat_output.close();
		sleep (10);
	}
}

