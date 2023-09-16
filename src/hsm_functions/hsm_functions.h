#ifndef HSM_FUNCTIONS_H
#define HSM_FUNCTIONS_H


extern "C" {
	#include "common.h"
	#include "cloudhsm_pkcs11_vendor_defs.h"
	#include "cryptoki.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AES_GCM_IV_SIZE 12
#define AES_GCM_TAG_SIZE 16

CK_RV open_sessions(CK_SESSION_HANDLE * sessions, int len);

CK_RV pkcs11_open_session_2(CK_UTF8CHAR_PTR pin, CK_SESSION_HANDLE_PTR session);

CK_RV generate_aes_key(CK_SESSION_HANDLE session,
                       CK_ULONG key_length_bytes,
                       CK_OBJECT_HANDLE_PTR key);


CK_RV hsm_aes_encrypt(	CK_SESSION_HANDLE session, 
			CK_OBJECT_HANDLE wrapping_key, 
			CK_BYTE_PTR plaintext, 
			CK_ULONG plaintext_length, 
			CK_BYTE_PTR *ciphertext, 
			CK_ULONG *ciphertext_length);

CK_RV hsm_aes_decrypt(	CK_SESSION_HANDLE session, 
			CK_OBJECT_HANDLE wrapping_key, 
			CK_BYTE_PTR ciphertext, 
			CK_ULONG ciphertext_length, 
                        CK_BYTE_PTR *decrypted_ciphertext, 
                        CK_ULONG *decrypted_ciphertext_length);


CK_RV delete_old_k_master(  CK_SESSION_HANDLE session,
                            CK_OBJECT_HANDLE old_key);


CK_RV generate_new_root_key (   CK_SESSION_HANDLE session,
                                CK_ULONG key_length_bytes,
                                CK_OBJECT_HANDLE_PTR key);


#endif
