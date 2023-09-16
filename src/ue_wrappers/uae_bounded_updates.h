#ifndef UAE_BOUNDED_UPDATES_H
#define UAE_BOUNDED_UPDATES_H

#include "../ue_interface.h"
#include "../ue_schemes/uae_bounded_updates/AE_Nested_AES.h"

#include <string.h>
#include <stdio.h>

struct data_info {
    int num_updates;
};

struct decrypt_info {
    int num_updates;
    int ctx_length;
};

struct reencrypt_info {
    int ctx_length;
};


class uae_bounded_updates : public ue {
	public:
		uae_bounded_updates(int num_updates);
		int keygen (struct key* key_ret);
    		int enc (struct ctxt* ctxt_ret, struct key* key_ptr, struct ptxt* ptxt_ptr);	
		int dec (struct ptxt* ptxt_ret, struct key* key_ptr, struct ctxt* ctxt_ptr); 
		int prepare_tokgen (struct prep_tokgen_download* prep_ret);
		int tokgen (struct token* token_ret, struct key* key_ptr_1, struct key* key_ptr_2, struct tokgen_download* tokgen_download_ptr);
		int upd (struct ctxt* ctxt_ret, struct token* token_ptr, struct ctxt* ctxt_ptr);
	
	private:
		int num_reencrypts;
		int update_counter;
};

uae_bounded_updates::uae_bounded_updates(int num_updates) {
    num_reencrypts = num_updates;
    update_counter = 0;
}

// Function override keygen
int uae_bounded_updates::keygen (struct key* key_ret) {
    AE_key * ae_key = (AE_key*) malloc (sizeof(struct AE_key));
    AE_KeyGen(ae_key, num_reencrypts);
    key_ret->keyptr = ae_key;
    key_ret->len = sizeof(struct AE_key);
    return 0;
}

// Function override enc
int uae_bounded_updates::enc (struct ctxt* ctxt_ret, struct key* key_ptr, struct ptxt* ptxt_ptr){
    int total_re_encrypts = num_reencrypts;

    int size = ptxt_ptr->len;
    int buffer_length = sizeof(ct_hat_data_en) + sizeof(AE_ctx_header) + size + total_re_encrypts * (2 * RHO + NU);

    uint8_t * ctxt = (uint8_t *) malloc(buffer_length);
    int ctx_length = AE_Encrypt((AE_key*) key_ptr->keyptr, (uint8_t*) ptxt_ptr->ptxtptr, (ct_hat_data_en*) ctxt, ctxt + sizeof(ct_hat_data_en), size);
    ctxt_ret->ctxtptr = ctxt;
    ctxt_ret->len = buffer_length;

    return ctx_length;
}

// Function override dec
int uae_bounded_updates::dec (struct ptxt* ptxt_ret, struct key* key_ptr, struct ctxt* ctxt_ptr) {
    //printf ("uae_dec begin\n");
    int total_re_encrypts = num_reencrypts;
    int ctx_length = ctxt_ptr->len - sizeof(AE_ctx_header) - sizeof(ct_hat_data_en);

    int size = ctxt_ptr->len - sizeof(AE_ctx_header) - sizeof(ct_hat_data_en) - total_re_encrypts * (2 * RHO + NU);
    uint8_t * ptxt1 = (uint8_t *) malloc (size);
    AE_Decrypt((AE_key*) key_ptr->keyptr, (ct_hat_data_en*) ctxt_ptr->ctxtptr, (uint8_t*) (ctxt_ptr->ctxtptr + sizeof(ct_hat_data_en)), ptxt1, ctx_length);
    ptxt_ret->ptxtptr = ptxt1;
    ptxt_ret->len = size;
    return size;
}

int uae_bounded_updates::prepare_tokgen (struct prep_tokgen_download* prep_ret) {
    prep_ret->start_index = 0;
    
    if (get_num_updates()%(num_reencrypts + 1) == 0) {
   	prep_ret->end_index = -1;
	prep_ret->start_index = -1;	
    }
    else {
        prep_ret->end_index = sizeof(ct_hat_data_en);
    }
    return 0;
}

// Function override tokgen
int uae_bounded_updates::tokgen (struct token* token_ret, struct key* key_ptr_1, struct key* key_ptr_2, struct tokgen_download* tokgen_download_ptr) {
    
    if (get_num_updates()%(num_reencrypts + 1) == 0) {
        struct ptxt plaintext;;
        struct ctxt ciphertext;
        ciphertext.ctxtptr = tokgen_download_ptr->downloadptr;
        ciphertext.len = tokgen_download_ptr->len;
        int a = dec (&plaintext, key_ptr_1, &ciphertext);
        a = enc (&ciphertext, key_ptr_2, &plaintext);
        token_ret->tokptr = ciphertext.ctxtptr;
        token_ret->len = ciphertext.len;
        update_counter = 0;
        free (plaintext.ptxtptr);
    }
    else {
        struct ct_hat_data_en* ptr = (struct ct_hat_data_en*) (tokgen_download_ptr->downloadptr);
    
        delta_token_data* delta = (delta_token_data*) malloc (sizeof(delta_token_data)); 
        token_ret->tokptr = delta;
        token_ret->len = sizeof (delta_token_data);
        AE_ReKeyGen((AE_key*) key_ptr_1->keyptr, (AE_key*) key_ptr_2->keyptr, ptr, delta);
    }

    return 0;
}

// Function override upd
int uae_bounded_updates::upd (struct ctxt* ctxt_ret, struct token* token_ptr, struct ctxt* ctxt_ptr) {

    int ret_length = 0;
    if (token_ptr->len > sizeof (delta_token_data)) {
        uint8_t* new_ctxt = (uint8_t *)malloc(token_ptr->len);
        memcpy (new_ctxt, token_ptr->tokptr, token_ptr->len);
        ctxt_ret->ctxtptr = new_ctxt;
        ctxt_ret->len = token_ptr->len;
        ret_length = ctxt_ret->len;
    }
    else {
        int ctx_length = ctxt_ptr->len - sizeof(AE_ctx_header) - sizeof(ct_hat_data_en);

        uint8_t* new_ctxt = (uint8_t *)malloc(ctxt_ptr->len);
        ctxt_ret->ctxtptr = new_ctxt;
        ctxt_ret->len = ctxt_ptr->len;
        ret_length = AE_ReEncrypt((delta_token_data*) token_ptr->tokptr, (ct_hat_data_en*) ctxt_ptr->ctxtptr, (uint8_t*) (ctxt_ptr->ctxtptr + sizeof(ct_hat_data_en)), (ct_hat_data_en*) new_ctxt, new_ctxt + sizeof(ct_hat_data_en), ctx_length);
    }

    return 0;
}

#endif
