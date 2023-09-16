

#include "actions.h"
#include "../utils.h"


CK_RV encrypt_and_upload( CK_SESSION_HANDLE session, 
                        CK_OBJECT_HANDLE wrapping_key_handle, 
                        struct wrap* ret_wrap,
                        struct ctxt* ret_ctxt,
                        struct ptxt* ptxt, 
                        ue* scheme) {

    // Keygen and Encrypt
    struct key key1;
    scheme->keygen(&key1);
    scheme->enc(ret_ctxt, &key1, ptxt);

    // Wrap Key
    CK_BYTE_PTR wrapped_key = NULL;
    CK_ULONG wrapped_len = 0;

    CK_RV rv = hsm_aes_encrypt(session, wrapping_key_handle, (CK_BYTE_PTR)(key1.keyptr), (CK_ULONG)(key1.len), &wrapped_key, &wrapped_len);


    if (rv != CKR_OK) {
	printf("HSM Aes Encrypt Failed\n");
        free(wrapped_key);
        free (key1.keyptr);
	return rv;
    }

    // Return Values
    ret_wrap->wrapptr = wrapped_key;
    ret_wrap->len = wrapped_len;

    free (key1.keyptr);
    return rv;

}



CK_RV download_and_decrypt (  CK_SESSION_HANDLE session, 
                            CK_OBJECT_HANDLE wrapping_key_handle,
                            struct ptxt* ret_ptxt, 
                            struct wrap* wrapped_key, 
                            struct ctxt* ctxt, 
                            ue* scheme) {

    // Setup Scheme

    //// Unwrap Key
    CK_BYTE_PTR decryption_key = NULL;
    CK_ULONG decryption_key_length = 0;
    CK_RV rv = hsm_aes_decrypt(session, wrapping_key_handle, (CK_BYTE_PTR)(wrapped_key->wrapptr), (CK_ULONG)(wrapped_key->len), &decryption_key, &decryption_key_length);   
    if (rv != CKR_OK) {
        printf("Can't complete decrypt if hsm_aes_decrypt fails\n");
        return rv;
    }

    // Decrypt
    struct key key;
    key.keyptr = decryption_key;
    key.len = decryption_key_length;
    scheme->dec(ret_ptxt, &key, ctxt);
    free(key.keyptr);

    // Return Values
    return rv;
}



int perform_update (    CK_SESSION_HANDLE session, 
                        CK_OBJECT_HANDLE wrapping_key_handle,
			CK_OBJECT_HANDLE new_wrapping_key_handle,
			struct wrap* ret_new_wrap,
                        struct token* ret_new_token, 
                        struct wrap* wrapped_old_key, 
                        struct tokgen_download* tdownload, 
                        ue* scheme) {

    // Setup Scheme
    CK_RV rv;

    // Unwrap Old Key
    CK_BYTE_PTR decryption_key = NULL;
    CK_ULONG decryption_key_length = 0;
    rv = hsm_aes_decrypt(session, wrapping_key_handle, (CK_BYTE_PTR)(wrapped_old_key->wrapptr), (CK_ULONG)(wrapped_old_key->len), &decryption_key, &decryption_key_length);

    // Setup Key1
    struct key key1;
    key1.keyptr = decryption_key;
    key1.len = decryption_key_length;

    // Create new Key
    struct key key2;
    scheme->keygen(&key2);


    // // Create a new token
    scheme->tokgen(ret_new_token, &key1, &key2, tdownload);


    // Wrap New Key
    CK_BYTE_PTR wrapped_new_key = NULL;
    CK_ULONG wrapped_new_len = 0;
    rv = hsm_aes_encrypt(session, new_wrapping_key_handle, (CK_BYTE_PTR)(key2.keyptr), (CK_ULONG)(key2.len), &wrapped_new_key, &wrapped_new_len);

    ret_new_wrap->wrapptr = wrapped_new_key;
    ret_new_wrap->len = (int)wrapped_new_len;

    // Return New Key
    free (key1.keyptr);
    free (key2.keyptr);
}
