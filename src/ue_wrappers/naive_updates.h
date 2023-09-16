#ifndef NAIVE_UPDATES_H
#define NAIVE_UPDATES_H

#include "../ue_interface.h"
#include "../ue_schemes/utils/aes_gcm.h"
#include "../ue_schemes/uae_bounded_updates/AE_Nested_AES.h"

#include <openssl/rand.h>
#include <string.h>
#include <stdio.h>


class naive_updates : public ue {
	public:
		naive_updates();
		int keygen (struct key* key_ret);
    		int enc (struct ctxt* ctxt_ret, struct key* key_ptr, struct ptxt* ptxt_ptr); 
		int dec (struct ptxt* ptxt_ret, struct key* key_ptr, struct ctxt* ctxt_ptr); 
		int prepare_tokgen (struct prep_tokgen_download* prep_ret);
		int tokgen (struct token* token_ret, struct key* key_ptr_1, struct key* key_ptr_2, struct tokgen_download* tokgen_download_ptr);
		int upd (struct ctxt* ctxt_ret, struct token* token_ptr, struct ctxt* ctxt_ptr);
};

naive_updates::naive_updates() {
}

// Function override keygen
int naive_updates::keygen (struct key* key_ret) {
    
    uint8_t * ae_key = (uint8_t*) malloc (16);
    RAND_bytes(ae_key, 16);
    key_ret->keyptr = ae_key;
    key_ret->len = 16;
    return 0;
}

// Function override enc
int naive_updates::enc (struct ctxt* ctxt_ret, struct key* key_ptr, struct ptxt* ptxt_ptr){

    int size = ptxt_ptr->len;
    int buffer_length = 32 + size;


    uint8_t * ciphertext = (uint8_t *) malloc(buffer_length);
    AE_ctx_header * ctxt = (AE_ctx_header * ) ciphertext;
    RAND_bytes(ciphertext, IV_LEN);


    gcm_encrypt((uint8_t * )(ptxt_ptr->ptxtptr), size, (uint8_t*)key_ptr->keyptr, ctxt->iv, IV_LEN, (uint8_t * )(ciphertext+sizeof(AE_ctx_header)), ctxt->tag);
    ctxt_ret->ctxtptr = ciphertext;
    ctxt_ret->len = buffer_length;

    return buffer_length;
}

// Function override dec
int naive_updates::dec (struct ptxt* ptxt_ret, struct key* key_ptr, struct ctxt* ctxt_ptr) {
    
    AE_ctx_header * downloaded_ctxt = (AE_ctx_header * ) ctxt_ptr->ctxtptr;
    uint8_t * decrypted_message = (uint8_t * ) malloc(ctxt_ptr->len-32);
    int decrypted_message_length = gcm_decrypt((uint8_t * )(((uint8_t*)(ctxt_ptr->ctxtptr))+sizeof(AE_ctx_header)), (ctxt_ptr->len)-32, (uint8_t * )(downloaded_ctxt->tag), (uint8_t*)(key_ptr->keyptr), (uint8_t * )(downloaded_ctxt->iv), IV_LEN, decrypted_message);
    ptxt_ret->ptxtptr = decrypted_message;
    ptxt_ret->len = decrypted_message_length;
    return decrypted_message_length;
}

int naive_updates::prepare_tokgen (struct prep_tokgen_download* prep_ret) {
    prep_ret->start_index = -1;
    prep_ret->end_index = -1;
    // This means to give me the entire ciphertext
    return 0;
}

// Function override tokgen
int naive_updates::tokgen (struct token* token_ret, struct key* key_ptr_1, struct key* key_ptr_2, struct tokgen_download* tokgen_download_ptr) {
    
    struct ctxt ctxt1;
    ctxt1.ctxtptr = tokgen_download_ptr->downloadptr;
    ctxt1.len = tokgen_download_ptr->len;
    struct ptxt ptxt1;
    int ptxt1_size = dec (&ptxt1, key_ptr_1, &ctxt1);

    if (ptxt1_size == -1) {
        printf("ERROR, ptxtsize is %d. Changing to %d\n", ptxt1_size, backup_plaintext_size);
        fflush(stdout);
        ptxt1_size = backup_plaintext_size;
        ptxt1.len = backup_plaintext_size;
    }

    struct ctxt ctxt2;
    enc (&ctxt2, key_ptr_2, &ptxt1);
    token_ret->tokptr = ctxt2.ctxtptr;
    token_ret->len = ctxt2.len;

    free(ptxt1.ptxtptr);

    return 0;
}

// Function override upd
int naive_updates::upd (struct ctxt* ctxt_ret, struct token* token_ptr, struct ctxt* ctxt_ptr) {
    ctxt_ret->ctxtptr = (void*) malloc (token_ptr->len);
    memcpy (ctxt_ret->ctxtptr, token_ptr->tokptr, token_ptr->len);
    ctxt_ret->len = token_ptr->len;

    return 0;
}

#endif
