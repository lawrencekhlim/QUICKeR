
#ifndef UTIL_H
#define UTIL_H

#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

double get_time_in_seconds();

char** split_by_space (char *str, int* count, int stringlen);

void free_split_by_space (char** string_arr);

char cksum(const char* input, int length);

int recvall (int sockfd, void* recvbuf, int buffsize);

int sendall (int sockfd, void* sendbuf, int sendsize);

char *base64_dec(char *str, int len, int *result_len);

char *base64_enc(const char *str, int len);

void** parse_string (void* to_parse, int num_entries, int** lengths); 

int put_string (void* buff, int num_entries, int* lengths, void** vals);

#endif

