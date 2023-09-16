#include "utils.h"
#include "ue_interface.h" 
#include "hsm_functions/hsm_functions.h"
#include "actions/actions.h"
#include "actions/client_actions.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
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

#define MAX 10000000
#define SA struct sockaddr



int timer_expired = 0;
int num_ops = 0;

char* server_ip_addr;
int PORT = 7000;
CK_SESSION_HANDLE* sessions;
CK_OBJECT_HANDLE root_key;


char* ue_scheme_name;
ue** thread_schemes;
pthread_mutex_t lock;
int plaintext_size = 0;
int num_messages;
char* filename;
int num_threads = 1;

int* kv_key_list;


pthread_mutex_t data_lock;


int backup_plaintext_size = 100;



static void* myThreadFun(void* t_num) {

    int thread_num = *(int*)(t_num);
    int thread_port = PORT + thread_num;
    plaintext = (char*) malloc (plaintext_size);

    CK_SESSION_HANDLE sess = sessions[thread_num];
    ue* ue_scheme = thread_schemes[thread_num];

    memset (plaintext, 'g', plaintext_size);
    struct ptxt p;
    p.ptxtptr = (void*)plaintext;
    p.len = plaintext_size;

    struct ip_port conn_params;
    conn_params.ip_addr = server_ip_addr;
    conn_params.port = thread_port;



    ////////////ENCRYPT HERE//////////////////////
    printf(">>>>>>Begin Encrypt\n");
    char kv_key_buff[10];


    for (int i = thread_num; i < num_messages; i+=num_threads) {
        // secure put here
        bzero (kv_key_buff, 10);    

    
        sprintf (kv_key_buff, "%d", i);
        kv_key_list[i] = i;

        secureput (conn_params, sess, ue_scheme, root_key, kv_key_buff, &p);
    }
    printf(">>>>>>Finish Encrypt for thread: %d\n", thread_num);
    return 0;
}









// Driver function
int main(int argc, char** argv)
{
    if (argc < 8) {
        printf ("Usage: ./client <ue scheme> <ptxt size> <num messages> <num_threads> <server_ip_addr> <server_port> <ctxt_out>\n");
        exit (0);
    }
    ue_scheme_name = argv[1];
    char* tmpptr;

    // // open file to write to
    // std::ofstream myfile2;
    // myfile2.open (argv[7]);
    // char kv_key_buff2[10];
    // for (int i=0; i < strtol(argv[3], &tmpptr, 10); i++) {
    //     bzero (kv_key_buff2, 10);        
    //     sprintf (kv_key_buff2, "%d", i);
    //     myfile2 << kv_key_buff2 << "\n";
    // }
    // myfile2 << kv_key_buff2 << "\n";
    // myfile2.close();

    // exit(0);


    double start = get_time_in_seconds();

    // OPEN SESSIONS
    num_threads = strtol(argv[4], &tmpptr, 10);
    printf("NUM THREADS: %d\n", num_threads);
    sessions = (CK_SESSION_HANDLE*) malloc ((num_threads + 2) * sizeof (CK_SESSION_HANDLE));  
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
    open_sessions(sessions, num_threads + 2);
    // only need to login once for all sessions
    printf("session1: Logging in to slot via session1.\n");
    CK_RV rv = funcs->C_Login((sessions[0]), CKU_USER, (CK_UTF8CHAR_PTR)args.pin, (CK_ULONG)strlen(args.pin));
    if (rv != CKR_OK) {
        fprintf(stderr, "session1: Failed to login.\n");
        return rv;
    }
    // OPEN SESSION

    
    plaintext_size = strtol(argv[2], &tmpptr, 10);
    num_messages = strtol(argv[3], &tmpptr, 10);
    server_ip_addr = argv[5];
    PORT = strtol (argv[6], &tmpptr, 10);
    filename = argv[7];

    // Generate a new root key
    char small_buff[100];
    bzero (small_buff, 100);
    rv = generate_new_root_key (sessions[0], 16, &root_key);
    printf ("Current root key id = %lu\n", root_key);
    // Generate new root key

    struct ip_port conn_params;
    conn_params.ip_addr = server_ip_addr;
    conn_params.port = PORT;

    /////////////SENDING ROUND/////////////////////
    const char* round_input = "ROUND ";
    
    memcpy(small_buff, round_input, strlen (round_input));
    int len_kv_key = sizeof(CK_OBJECT_HANDLE);
    void** to_send = (void**)malloc (sizeof (void*) * 1); 
    to_send[0] = &root_key;
    int extra_offset = put_string (&(small_buff[6]), 1, &len_kv_key, to_send);
    //strncat(buff, kv_key, strlen (kv_key));

    int sockfd = connect_to_server(conn_params);

    sendall(sockfd, small_buff, extra_offset + strlen(round_input));
    free (to_send);
    bzero(small_buff, 100);
    
    int recvlen = recvall(sockfd, small_buff, sizeof(small_buff));
    printf ("Got back for ROUND: %s\n", small_buff);
    /////////////SENDING ROUND/////////////////////

    kv_key_list = (int*)malloc(sizeof(int) * num_messages);



    pthread_t* tid = (pthread_t*) malloc (num_threads * sizeof (pthread_t));

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    thread_schemes = (ue**) malloc (sizeof (ue*) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        thread_schemes[i] = updatable_encryption_function (ue_scheme_name);
    }

    for (int i = 0; i < num_threads; i++) {
        int* thread_num = (int*) malloc(sizeof(int));
        *thread_num = i;
        pthread_create(&(tid[i]), NULL, &myThreadFun, (void*) thread_num);
    }


    for (int i = 0; i < num_threads; i++) {
        pthread_join(tid[i], NULL);
    }
    pthread_mutex_destroy(&lock);


    // open file to write to
    std::ofstream myfile;
    myfile.open (filename);
    char kv_key_buff[10];
    for (int i=0; i < num_messages; i++) {
        bzero (kv_key_buff, 10);        
        sprintf (kv_key_buff, "%d", kv_key_list[i]);
        myfile << kv_key_buff << "\n";
    }
    myfile << kv_key_buff << "\n";
    myfile.close();


    double end = get_time_in_seconds();
    printf("end - start: %f\n", (end-start));


    // int arr[] = { };
    // for (int i = 0; i < sizeof (arr) / 4; i++) {        
    //     CK_RV t = delete_old_k_master(sessions[0], arr[i]);
    // }


    
}









