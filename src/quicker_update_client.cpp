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
#include "update_queue/update_queue.h"
#include "hsm_functions/hsm_functions.h"
#include "actions/actions.h"
#include "actions/client_actions.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include <fstream>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
//#define MAX 10000000
//#define MAX 10000

int timer_expired = 0;
int num_ops = 0;
int num_rounds = 0;
pthread_mutex_t lock;


char* ue_scheme_name;
char* server_ip_addr;
int PORT = 7200;
int num_threads = 10;
std::string filename;
CK_OBJECT_HANDLE old_root_key_id = 0;
CK_SESSION_HANDLE* sessions;
ue** thread_schemes;

int backup_plaintext_size = 100;

static void* myThreadFun(void* t_num) {
	int thread_num = *(int*)(t_num);
	int thread_port = PORT + thread_num;
	CK_SESSION_HANDLE sess = sessions[thread_num];
	ue* ue_scheme = thread_schemes[thread_num];
	if (thread_num == 5) {
        sess = sessions[num_threads+1];
    } else if (thread_num == 6) {
        sess = sessions[num_threads + 2];
    }

	struct ip_port conn_param;
	conn_param.ip_addr = server_ip_addr;
       	conn_param.port = thread_port;

	printf("Thread %d connecting to port %d\n", thread_num, thread_port);	

	int counter = 0;
	char kv_key_buff[10];
	while (timer_expired == 0) {

		// dequeue
		std::string key_to_update = dequeue();	
		bzero (kv_key_buff, 10);
		memcpy (kv_key_buff, key_to_update.c_str(), key_to_update.length());

		update(conn_param, sess, ue_scheme, current_root_key_id, kv_key_buff);
		
		counter += 1;
	}

	pthread_mutex_lock(&lock);
	num_ops += counter;
	pthread_mutex_unlock(&lock);
}

static void* new_round_thread (void* arg) {
	int thread_port = PORT + num_threads;

	struct ip_port conn_param;
	conn_param.ip_addr = server_ip_addr;
       	conn_param.port = thread_port;	
	
	char smallbuff[100];
	while (timer_expired == 0) {
		bzero (smallbuff, 100);
		memcpy (smallbuff, "ROUND ", 6);
		all_waiting();
		for (int i = 0; i < num_threads; i++) {
			thread_schemes[i]->increment_num_updates();
		}
		CK_OBJECT_HANDLE tmp = old_root_key_id;	
		old_root_key_id = current_root_key_id;
		CK_RV i = generate_new_root_key (sessions[num_threads], 16, &current_root_key_id);
        
		// tell the database of the new root key
		int lengths[1] = { sizeof (CK_OBJECT_HANDLE) };
		void* vals[1] = { &current_root_key_id };
		int extra_len = put_string(&(smallbuff[6]), 1, lengths, vals);
		
		int sockfd = connect_to_server(conn_param);
		
		int rv = sendall (sockfd, smallbuff, extra_len + 6);

		bzero (smallbuff, 100);

		rv = recvall (sockfd, smallbuff, 100);
		close (sockfd);
		if (tmp != 0) {
			printf ("delete_old_k_master\n");
			printf ("tmp = %d\n", tmp);
			i = delete_old_k_master( sessions[num_threads], tmp );
		}
		enqueue_all(filename);
		num_rounds ++;
	}
}


int main (int argc, char** argv) {

    	if (argc < 9) {
		printf ("Usage: ./quicker_update_client <ue scheme> <ptxt size> <num_threads> <server_ip_addr> <server_port> <root_key> <key-list-file>\n");
		exit (0);
    	}
	ue_scheme_name = argv[1];

	char* tmpptr;
	ptxt_size = strtol(argv[2], &tmpptr, 10);
	backup_plaintext_size = ptxt_size;
	plaintext = (char*) malloc (ptxt_size * 1.2);
	memset (plaintext, 'b', ptxt_size);
	
	num_threads = strtol(argv[3], &tmpptr, 10) ;
	// OPEN SESSIONS
	sessions = (CK_SESSION_HANDLE*) malloc ((num_threads + 3) * sizeof (CK_SESSION_HANDLE));	
	struct pkcs_arguments args = {0};
	// parse command line args for pkcs11 and login to HSM
  	if (get_pkcs_args(argc, argv, &args) < 0) {
    		return EXIT_FAILURE;
  	}
	printf ("args.library = %s\n", args.library);
	// Initialization of pkcs11 library
  	if( pkcs11_initialize(args.library) != CKR_OK) {
    		printf ("pkcs11 lib initialization failure\n");
		return EXIT_FAILURE;
  	}
	// open all sessions
  	open_sessions(sessions, num_threads + 3);
    
  	// only need to login once for all sessions
  	printf("session1: Logging in to slot via session1.\n");
  	CK_RV rv = funcs->C_Login((sessions[0]), CKU_USER, (CK_UTF8CHAR_PTR)args.pin, (CK_ULONG)strlen(args.pin));
  	if (rv != CKR_OK) {
    		fprintf(stderr, "session1: Failed to login.\n");
    		return rv;
  	}
  	// OPEN SESSION
	
	server_ip_addr = argv[4];
	PORT = strtol (argv[5], &tmpptr, 10);

	current_root_key_id = strtol (argv[6], &tmpptr, 10);

	filename = std::string(argv[7]);

	pthread_t* tid = (pthread_t*) malloc (num_threads * sizeof (pthread_t));

	init (num_threads);


    	if (pthread_mutex_init(&lock, NULL) != 0) {
        	printf("\n mutex init has failed\n");
        	return 1;
    	}

	thread_schemes = (ue**) malloc (num_threads * sizeof (ue*));
	for (int i = 0; i < num_threads; i++) {
		thread_schemes[i] = updatable_encryption_function (ue_scheme_name);
	}

	for (int i = 0; i < num_threads; i++) {
		int* thread_num = (int*) malloc(sizeof(int));
		*thread_num = i;
		pthread_create(&(tid[i]), NULL, &myThreadFun, (void*) thread_num);
	}

	/* create new round thread*/
	pthread_t round_coordinator;
	pthread_create (&round_coordinator, NULL, &new_round_thread, (void*) 0);

	//timer_expired = 0;
	sleep (200);
	timer_expired = 1;
	for (int i = 0; i < num_threads; i++) {
		pthread_join(tid[i], NULL);
	}
	pthread_mutex_destroy(&lock);
	printf ("ops: %d\n", num_ops);
	printf ("num_rounds: %d\n", num_rounds);
	return 0;
}
