#include "utils.h"


double get_time_in_seconds() {
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) + (tv.tv_usec) / 1000000.0 ;
}


char *base64_enc(const char *str, int len)
{
	BIO *bio, *b64;
	BUF_MEM *bptr;
	char *buf;
	int ret;


	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);


	ret = BIO_write(bio, str, len);
	if (ret <= 0) {
		buf = NULL;
		goto err;
	}
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	buf = (char*)malloc(bptr->length + 1);
	if (buf) {
		memcpy(buf, bptr->data, bptr->length+1);
		buf[bptr->length] = 0;
	}

err:
	BIO_free_all(b64);

	return buf;
}


char *base64_dec(char *str, int len, int *result_len) {
	BIO *bio, *b64;
	char *buf;
	int ret;

	if (!(buf = (char*)malloc(len + 1)))
		return NULL;

	memset(buf, 0, len + 1);

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new_mem_buf(str, len);
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	ret = BIO_read(bio, buf, len);
	if (ret <= 0) {
		free(buf);
		buf = NULL;
	}   
	BIO_free_all(bio);


	if (result_len)
		*result_len = ret;

	return buf;
}

char** split_by_space (char *str, int* count, int stringlen) {

	int length = stringlen;
	char* token;

	char* copy = (char*) malloc (length + 1);
	memset (copy, 0, length + 1);
	memcpy (copy, str, length);
	char* rest = copy;
	(*count) = 0;
	while ((token = strtok_r(rest, " ", &rest))) {
		(*count)++;
	}

	char** string_arr = (char**) malloc(*count * sizeof(char*));
	memset(copy, 0, length+1);
	strncpy(copy, str, length);
	rest = copy;
	int index = 0;
	while ((token = strtok_r(rest, " ", &rest))) {
		string_arr[index] = token;
		index ++;
	}

	return string_arr;
}

void free_split_by_space (char** string_arr) {
	free (string_arr[0]);
	free (string_arr);
}

char cksum(const char* input, int length) {
	
    	char check = 0;
    	for(int i = 0; i != length; ++i) {
        	check ^= input[i];
    	}
    	return check;

}

int recvall (int sockfd, void* recvbuf, int buffsize) {
	//bzero (recvbuf, buffsize);
	int total_bytes = 0;
	int nbytes = 0;
	char integerbuf[4];
	void* intbuf = (void*) integerbuf;

	int index = 0;
	
	// First receive four bytes indicating how many more bytes we will recv
	while (total_bytes < 4 && (nbytes = recv(sockfd, intbuf, 4, 0)) > 0){
		intbuf += nbytes;
		total_bytes += nbytes;
	}

	int bytes_to_recv = *((int*)integerbuf);
	if (bytes_to_recv > buffsize) {
		bytes_to_recv = buffsize;
	}

	void* startbuf = recvbuf;
	total_bytes = 0;
	nbytes = 0;
	while (total_bytes < bytes_to_recv) {
		nbytes = recv (sockfd, startbuf + total_bytes, bytes_to_recv - total_bytes, 0);
		total_bytes += nbytes;
	}
	return total_bytes;
}

int sendall (int sockfd, void* sendbuf, int sendsize) {
	int total_bytes = 0;
	int nbytes = 0;

	// First send four bytes indicating how many more bytes we will send

	int rem = 4;
	char integerbuf[4];
	void* intbuf = (void*) integerbuf;
	memcpy (intbuf, &sendsize, 4);
	while (rem > 0 && (nbytes = send(sockfd, intbuf, rem, 0)) > 0 ) {
		intbuf += nbytes;
		rem -= nbytes;
		total_bytes += nbytes;
	}

	void* startbuf = sendbuf;

	total_bytes = 0;
	int max_send = 50000;
	int current_send = sendsize; 
	if (current_send > max_send) {
		current_send = max_send;
	}

	while (sendsize > 0 && (nbytes = send(sockfd, startbuf, current_send, 0)) > 0 ) {
		startbuf += nbytes;
		sendsize -= nbytes;
		total_bytes += nbytes;

		current_send = sendsize; 
		if (current_send > max_send) {
			current_send = max_send;
		}
	}	
	return total_bytes;
}


void** parse_string (void* to_parse, int num_entries, int** lengths) {
	*lengths = (int*) malloc (num_entries * sizeof (int));
	int* len_arr = *lengths;

	int offset = 0;
	for (int i = 0; i < num_entries; i++) {
		offset = i * 4;
		memcpy ( &(len_arr[i]), to_parse + offset, 4);
	}

	offset += 4;

	void** ret = (void**) malloc (num_entries * sizeof (void*));
	for (int i = 0; i < num_entries; i++) {
		ret[i] = to_parse + offset;
		offset += len_arr[i];
	}
	return ret;	
}	

int put_string (void* buff, int num_entries, int* lengths, void** vals) {
	int offset = 0;
	for (int i = 0; i < num_entries; i++) {
		offset = i * 4;
		memcpy (buff + offset, &(lengths[i]), 4);
	}
	offset += 4;
	
	for (int i = 0; i < num_entries; i++) {
		memcpy (buff + offset, vals[i], lengths[i]);
		offset += lengths[i];
	}
	return offset;
}

