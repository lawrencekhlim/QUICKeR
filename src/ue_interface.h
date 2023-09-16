#ifndef UE_INTERFACE_H
#define UE_INTERFACE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

extern int backup_plaintext_size;

struct key {
    void* keyptr;
    int len;
};

struct ctxt {
    void* ctxtptr;
    int len;
};

struct ptxt {
    void* ptxtptr;
    int len;
};

struct info {
    void* infoptr;
    int len;
};

struct token {
    void* tokptr;
    int len;
};

struct prep_tokgen_download {
    int start_index;
    int end_index;
};

struct tokgen_download {
    void* downloadptr;
    int len;
};

class ue {
	private:
		int num_updates = 0;

	public:
		virtual int keygen (struct key* key_ret) = 0;
    		virtual int enc (struct ctxt* ctxt_ret, struct key* key_ptr, struct ptxt* ptxt_ptr) = 0;	
		virtual int dec (struct ptxt* ptxt_ret, struct key* key_ptr, struct ctxt* ctxt_ptr) = 0;
		virtual int prepare_tokgen (struct prep_tokgen_download* prep_ret) = 0;
		virtual int tokgen (struct token* token_ret, struct key* key_ptr_1, struct key* key_ptr_2, struct tokgen_download* tokgen_download_ptr) = 0;
		virtual int upd (struct ctxt* ctxt_ret, struct token* token_ptr, struct ctxt* ctxt_ptr) = 0;
		void increment_num_updates () { num_updates++; }
		int get_num_updates() { return num_updates; }
	

};

ue* updatable_encryption_function (const char* arg);

#endif
