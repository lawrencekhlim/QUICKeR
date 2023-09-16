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
#include <vector>
#include <random>

#define SA struct sockaddr

int num_threads;
int timer_expired = 0;
int num_ops_write = 0;
int num_ops_read = 0;
int num_ops_fail = 0;
pthread_mutex_t lock;

char* ue_scheme_name;
char* server_ip_addr;
int PORT = 7000;
int seed = 0;
std::vector<double> weights;

CK_SESSION_HANDLE* sessions;
std::vector <std::string> key_list;
std::vector <int> key_hitrate;

int backup_plaintext_size = 100;

static void* myThreadFun(void* t_num) {
    ue* ue_scheme = updatable_encryption_function (ue_scheme_name);

    int thread_num = *(int*)(t_num);
    int thread_port = PORT + thread_num;
    int thread_seed = seed + thread_num;

    CK_SESSION_HANDLE sess = sessions[thread_num];
    if (thread_num == 5) {
        sess = sessions[num_threads];
    } else if (thread_num == 6) {
        sess = sessions[num_threads + 1];
    }

    struct ip_port conn_param;
    conn_param.ip_addr = server_ip_addr;
    conn_param.port = thread_port;

    printf("Thread %d connecting to port %d\n", thread_num, thread_port);

    char keybuff[10];
    srand (time(NULL));

    // Random dist and rng for selecting key index
    std::mt19937 key_rng(thread_seed);
    std::discrete_distribution<> key_dist(weights.begin(), weights.end());

    // Random dist and rng for selecting puts vs gets operations
    std::mt19937 ops_rng(thread_seed);
    std::discrete_distribution<> ops_dist({read_portion,1-read_portion});

    int read_counter = 0;
    int write_counter = 0;
    int fail_counter = 0;
    while (timer_expired == 0) {
        int random = ops_dist(ops_rng);
        int random2 = key_dist(key_rng);
        bzero (keybuff, 10);
        memcpy(keybuff, key_list[random2].c_str(), strlen(key_list[random2].c_str()));
        key_hitrate[random2] += 1;
        
        //printf("%d Beginning SecurePUT or SecureGet\n---------------------------------------\n", thread_num);
        if (random == 1) {
            //printf ("write %s\n", keybuff);
            double start_time = get_time_in_seconds();
            struct ptxt p;
            p.ptxtptr = plaintext;
            p.len = ptxt_size;
            secureput (conn_param, sess, ue_scheme, current_root_key_id, keybuff, &p);
            write_counter++;
            double  end_time = get_time_in_seconds();
            //printf("Write Time: %f\n", (end_time - start_time));
        }
        else {
            double start_time = get_time_in_seconds();
            //printf ("read  %s\n", keybuff);
            //printf ("Get\n");
            struct ptxt p2;
            
            secureget (conn_param, sess, ue_scheme, keybuff, &p2);
            
            //printf ("got back %s\n", p2.ptxtptr);
            free (p2.ptxtptr);
            read_counter ++;
            double  end_time = get_time_in_seconds();
            //printf("Read Time: %f\n", (end_time - start_time));
        }
        if ((read_counter + write_counter) % 2000 == 0) {
            printf ("%d: read_counter:  %d\n",  thread_num, read_counter);
            printf ("%d: write_counter: %d\n", thread_num, write_counter);
        }
        //printf("%d -----------------------------------------------\n", thread_num);
    }
    //printf("Thread %d Has completed\n", thread_num);
    pthread_mutex_lock(&lock);
    num_ops_write += write_counter;
    num_ops_read += read_counter;
    pthread_mutex_unlock(&lock);
    //free (ue_scheme);
    //free (cryptokey.keyptr);
}

// Driver function
int main(int argc, char** argv)
{
        if (argc < 9) {
        printf ("Usage: ./client <ue scheme> <ptxt size> <num_threads> <server_ip_addr> <server_port> <read_proportion (float)> <root_key> <seed> <key-list-file>\n");
        exit (0);
        }
    ue_scheme_name = argv[1];

    char* tmpptr;
    seed = strtol(argv[8], &tmpptr, 10);
    ptxt_size = strtol(argv[2], &tmpptr, 10);
    plaintext = (char*) malloc (ptxt_size);
    memset (plaintext, 'b', ptxt_size);
    
    num_threads = strtol(argv[3], &tmpptr, 10);
    
    // OPEN SESSIONS
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
    
    server_ip_addr = argv[4];
    PORT = strtol (argv[5], &tmpptr, 10);
    read_portion = atof (argv[6]);

    current_root_key_id = strtol (argv[7], &tmpptr, 10);

    key_list = std::vector<std::string>();
    std::ifstream myfile;
    myfile.open (argv[9]);
    std::string line;
    while (std::getline(myfile, line))
    {
        std::string trimmed_line = trim (line);
        //printf ("%s\n", trimmed_line.c_str());
        key_list.push_back (trimmed_line);
    }

    key_hitrate = std::vector<int>(key_list.size());
    std::fill(key_hitrate.begin(), key_hitrate.end(), 0);

    // Random Generator Params
    int num_elements = key_list.size();
    double zipf_parameter = 1.0;

    // Generate the Zipfian distribution weights
    weights.resize(num_elements);
    for (int i = 0; i < num_elements; ++i) {
        weights[i] = 1.0 / std::pow(i + 1, zipf_parameter);
    }


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
    printf ("write_ops: %d\n", num_ops_write);
    printf ("read_ops: %d\n", num_ops_read);
    for(int i=0; i < key_hitrate.size(); i++) {
        if (key_hitrate[i] > 0) {
            printf("Key %d: %d hits\n", i, key_hitrate[i]);
        }
    }

    return 0;
}
