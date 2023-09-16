#ifndef CLIENT_ACTIONS_H
#define CLIENT_ACTIONS_H


#include "../utils.h"
#include "../ue_interface.h" 
#include "../update_queue/update_queue.h"
#include "../hsm_functions/hsm_functions.h"
#include "actions.h"

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

#define MAX 10000000



struct ip_port {
	char* ip_addr;
	int port;
};

extern int ptxt_size;
extern char* plaintext;
extern double read_portion;
extern CK_OBJECT_HANDLE current_root_key_id;


int connect_to_server(struct ip_port vals);

// kv_key must be null terminated string
void secureget(struct ip_port sockfd, CK_SESSION_HANDLE session, ue* ue_scheme, char* kv_key, struct ptxt* ret_ptxt);

// kv_key must be null terminated
void secureput (struct ip_port sockfd, CK_SESSION_HANDLE session, ue* ue_scheme, CK_OBJECT_HANDLE root_key, char* kv_key, ptxt* pltxt);

// kv_key must be null terminated
void update(struct ip_port sockfd, CK_SESSION_HANDLE session, struct ue* ue_scheme, CK_OBJECT_HANDLE new_root_key, char* kv_key);

#endif
