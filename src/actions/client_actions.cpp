#include "client_actions.h"
#define SA struct sockaddr


int ptxt_size = 100;
char* plaintext = NULL;
double read_portion = 0.5;
CK_OBJECT_HANDLE current_root_key_id = 0;

int connect_to_server(struct ip_port vals) {
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
	servaddr.sin_addr.s_addr = inet_addr(vals.ip_addr);
	servaddr.sin_port = htons(vals.port);


	// connect the client socket to server socket
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed on port %d...\n", vals.port);
		exit(0);
	}
	else {
		//printf("connected to the server on port %d..\n", port);
	}
	return sockfd;
}

// kv_key must be null terminated string
void secureget(struct ip_port ip_port, CK_SESSION_HANDLE session, ue* ue_scheme, char* kv_key, struct ptxt* ret_ptxt)
{
	char* buff = (char*) malloc(sizeof(char) * MAX);
	int n;

	bzero(buff, MAX);
	n = 0;

	const char* input = "GET ";
	strncat(buff, input, strlen (input));
	int len_kv_key = strlen (kv_key) + 1;
	void* to_send[1] = { (void*)kv_key };
	int extra_offset = put_string (buff + 4, 1, &len_kv_key, to_send);


	double start_recvget = get_time_in_seconds();

	int s_fd = connect_to_server(ip_port);
	sendall(s_fd, buff, extra_offset + 4);

	bzero(buff, MAX);
	int recvlen = recvall(s_fd, buff, MAX);
	close(s_fd);


	double end_recvget = get_time_in_seconds();

	// parse server output
	int arraylen = 0;
	char ** split_vals = split_by_space (buff, &arraylen, recvlen);

	if (strncmp(split_vals[0], "RESULT", 6) == 0) {
	
		char* return_data = &(buff[7]);
		int *data_lengths;
		void** data_vals = parse_string (return_data, 3, &data_lengths);
		
		CK_OBJECT_HANDLE* root_key_id = (CK_OBJECT_HANDLE*)data_vals[0];
		uint8_t* wrapped_key_data = (uint8_t*)data_vals[1];
		uint8_t* ctxt_data = (uint8_t*)data_vals[2];

		double start_download = get_time_in_seconds();

		struct wrap wrapped_key;
		wrapped_key.wrapptr = wrapped_key_data;
		wrapped_key.len = data_lengths[1];
		struct ctxt ctxt; 
		ctxt.ctxtptr = ctxt_data;
		ctxt.len = data_lengths[2];	//
		CK_RV rv = download_and_decrypt ((session), *root_key_id, ret_ptxt, &wrapped_key, &ctxt, ue_scheme); 

		if (rv == CKR_OBJECT_HANDLE_INVALID) {
			printf("Redo SecureGet\n");
			secureget (ip_port, session, ue_scheme, kv_key, ret_ptxt);
		}


		double end_download = get_time_in_seconds();

		free (data_lengths);
		free (data_vals);
	}
	else {  // Restart fail
		printf("Entered restart fail\n");

		struct ptxt new_tmp_ptxt;
		new_tmp_ptxt.ptxtptr = (void*) plaintext;
		new_tmp_ptxt.len = ptxt_size;
		secureput (ip_port, session, ue_scheme, current_root_key_id, kv_key, &new_tmp_ptxt);
		secureget (ip_port, session, ue_scheme, kv_key, ret_ptxt);
	}
	free (buff);
	free (split_vals[0]);
	free (split_vals);
}


// kv_key must be null terminated
void secureput (struct ip_port ip_port, CK_SESSION_HANDLE session, ue* ue_scheme, CK_OBJECT_HANDLE root_key, char* kv_key, ptxt* pltxt) {
	char* buff = (char*) malloc(sizeof(char) * MAX);

	bzero(buff, MAX);
	double start_encrypt = get_time_in_seconds();

	struct ctxt ctxt;
	struct wrap key_wrap;
	int i = encrypt_and_upload((session), root_key, &key_wrap, &ctxt, pltxt, ue_scheme);

	double end_encrypt = get_time_in_seconds();

	int* send_lengths = new int[4];
	send_lengths[0] = strlen(kv_key) + 1;
	send_lengths[1] = sizeof(CK_OBJECT_HANDLE);
	send_lengths[2] = key_wrap.len;
	send_lengths[3] = ctxt.len;
	void** send_vals = (void**) malloc (4 * sizeof(void*));
	send_vals[0] = (void*) kv_key;
	send_vals[1] = (void*) &root_key;
	send_vals[2] = (void*) key_wrap.wrapptr;
	send_vals[3] = (void*) ctxt.ctxtptr;

	memcpy(buff, "PUT ", 4);
	int offset = put_string(&(buff[4]), 4, send_lengths, send_vals);
	int send_string_length = 4 + offset;


	double start_recvset = get_time_in_seconds();

	int s_fd = connect_to_server(ip_port);
	sendall (s_fd, buff, send_string_length);

	bzero (buff, MAX);
	int recvlen = recvall (s_fd, buff, MAX);
	close(s_fd);

	double end_recvset = get_time_in_seconds()
	
		
	int arraylen = 0;
	char ** split_vals = split_by_space (buff, &arraylen, recvlen);
	if (strncmp(split_vals[0], "ROOTKEY", 2) == 0) {
		int* data_lengths;
		void** data_vals = parse_string (&(buff[8]), 1, &data_lengths);	
		current_root_key_id = *((CK_OBJECT_HANDLE*)data_vals[0]);
		
		printf ("New current_root_key_id = %d\n", current_root_key_id);
		secureput (ip_port, session, ue_scheme, current_root_key_id, kv_key, pltxt);
		free (data_lengths);
		free (data_vals);	
	}

	free (ctxt.ctxtptr);
	free (send_vals);
	free (send_lengths);
	free (key_wrap.wrapptr);
	free (buff);
	free (split_vals[0]);
	free (split_vals);
}



void update(struct ip_port ip_port, CK_SESSION_HANDLE session, struct ue* ue_scheme, CK_OBJECT_HANDLE new_root_key, char* kv_key) {
	char* buff = (char*) malloc(sizeof(char) * MAX);
	bzero (buff, MAX);
	
	struct prep_tokgen_download ptd;
	ue_scheme->prepare_tokgen(&ptd);
	sprintf (buff, "PREP ");
	if (ptd.end_index == -1 || ptd.start_index == -1) {
		ptd.end_index = 0;
		ptd.start_index = 0;
	}
	void* vals [3] = { (void*)kv_key, (void*)&(ptd.start_index), (void*)&(ptd.end_index)  };
	int pack_len[3] = { strlen (kv_key) + 1, sizeof (int), sizeof (int) };
	int extra_len = put_string (&(buff[5]), 3, pack_len, vals);


	int s_fd = connect_to_server(ip_port);
	sendall (s_fd, buff, 5 + extra_len);
	bzero (buff, MAX);
	int recvlen = recvall (s_fd, buff, MAX);
	close(s_fd);

	int arraylen = 0;
	char ** split_vals = split_by_space (buff, &arraylen, recvlen);
	
	bool redisfail = (0 == strncmp (split_vals[0], "REDISFAIL", 6));

	if (strncmp (split_vals[0], "RESULT", 6) == 0) {

		int * lens;
		void** result_parse = parse_string (&(buff[7]), 4, &lens); 
		
		int v_num = (*(int*)result_parse[0]);
		CK_OBJECT_HANDLE tmp_root_key_id = (* (CK_OBJECT_HANDLE*)result_parse[1]);
		
		struct wrap old_wrap;
		old_wrap.wrapptr = result_parse[2];
		old_wrap.len = lens[2];

		struct tokgen_download downloaded;
		downloaded.downloadptr = result_parse[3];
		downloaded.len = lens[3];

		if (tmp_root_key_id == new_root_key) {
			// It is already the most recent key and we don't need to do anything
			
			free (split_vals[0]);
			free (split_vals);
			free (result_parse);
			free (lens);
			free (buff);
			return;
		}

		struct wrap new_wrap;
		struct token tok;	
		int ret_val = perform_update (session, tmp_root_key_id, new_root_key, &new_wrap, &tok, &old_wrap, &downloaded, ue_scheme); 

		void * vals2 [5] = {(void*)kv_key, (void*)&v_num, (void*)&new_root_key, (void*)new_wrap.wrapptr, (void*)tok.tokptr };
		int pack_len2[5] = { strlen(kv_key) + 1, sizeof (int), sizeof(CK_OBJECT_HANDLE), new_wrap.len, tok.len };
		bzero (buff, MAX);
		sprintf (buff, "UPD ");
		extra_len = put_string (&(buff[4]), 5, pack_len2, vals2);
	      		

		int s_fd2 = connect_to_server(ip_port);
		sendall (s_fd2, buff, 4 + extra_len);
		bzero (buff, MAX);
		int recvlen = recvall (s_fd2, buff, MAX);
		close(s_fd2);
		// Should receive either OK or REDISFAIL
		int arraylen = 0;
		char ** split_vals2 = split_by_space (buff, &arraylen, recvlen);
		
		redisfail = (0 == strncmp (split_vals2[0], "REDISFAIL", 6));
		free (split_vals2[0]);
		free (split_vals2);
		free (result_parse);
		free (lens);
		free (new_wrap.wrapptr);
		free (tok.tokptr);
	
	}

	if (redisfail) {
		struct ptxt p;
		p.ptxtptr = plaintext;
		p.len = ptxt_size;

		secureput (ip_port, session, ue_scheme, new_root_key, kv_key, &p);
	}

	free (split_vals[0]);
	free (split_vals);
	free (buff);
	
}


