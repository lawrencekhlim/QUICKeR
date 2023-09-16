#include "hsm_functions.h"


CK_RV open_sessions(CK_SESSION_HANDLE * sessions, int len) {

  // get the slot id to open a session on
  CK_SLOT_ID slot_id;
  CK_RV rv = pkcs11_get_slot(&slot_id);
  if (rv != CKR_OK) {
    fprintf(stderr, "Failed to get first slot.\n");
    return rv;
  }

  for (int i = 0; i < len; i++) {
    // open session i
    rv = funcs->C_OpenSession(slot_id, CKF_SERIAL_SESSION | CKF_RW_SESSION, NULL,
                            NULL, (&(sessions[i])));
    if (rv != CKR_OK) {
      fprintf(stderr, "Failed to open session %d.\n", i);
      return rv;
    }
  }
  return CKR_OK;
}

CK_RV pkcs11_open_session_2(const CK_UTF8CHAR_PTR pin, CK_SESSION_HANDLE_PTR session) {
    CK_RV rv;
    CK_SLOT_ID slot_id;

    if (!pin || !session) {
        return CKR_ARGUMENTS_BAD;
    }

    rv = pkcs11_get_slot(&slot_id);
    if (rv != CKR_OK) {
        return rv;
    }

    rv = funcs->C_OpenSession(slot_id, CKF_SERIAL_SESSION | CKF_RW_SESSION,
                              NULL, NULL, session);
    if (rv != CKR_OK) {
        return rv;
    }

    rv = funcs->C_Login(*session, CKU_USER, pin, (CK_ULONG) strlen((char*)pin));

    return rv;
}


CK_RV hsm_aes_encrypt( CK_SESSION_HANDLE session, 
                        CK_OBJECT_HANDLE wrapping_key,
                        CK_BYTE_PTR plaintext, 
                        CK_ULONG plaintext_length, 
                        CK_BYTE_PTR *ciphertext, 
                        CK_ULONG *ciphertext_length) {
    CK_RV rv;

    //CK_ULONG ciphertext_length = 0;
    CK_BYTE_PTR aad = (CK_BYTE_PTR)"plaintext aad";
    CK_ULONG aad_length = (CK_ULONG) strlen((char*)aad);

    // Prepare the mechanism
    CK_MECHANISM mech;
    CK_GCM_PARAMS params;

    // Allocate memory to hold the HSM generated IV.
    CK_BYTE_PTR iv = (CK_BYTE_PTR)malloc(AES_GCM_IV_SIZE);
    if (NULL == iv) {
        printf("Failed to allocate IV memory\n");
        return rv;
    }
    memset(iv, 0, AES_GCM_IV_SIZE);



    // Setup the mechanism with the IV location and AAD information.
    params.pIv = iv;
    params.ulIvLen = AES_GCM_IV_SIZE;
    params.ulIvBits = 0;
    params.pAAD = aad;
    params.ulAADLen = aad_length;
    params.ulTagBits = AES_GCM_TAG_SIZE * 8;

    mech.mechanism = CKM_AES_GCM;
    mech.ulParameterLen = sizeof(params);
    mech.pParameter = &params;

    //**********************************************************************************************
    // Encrypt
    //**********************************************************************************************
    rv = funcs->C_EncryptInit(session, &mech, wrapping_key);
    if (CKR_OK != rv) {
        printf("Encryption Init failed: %lu\n", rv);

        if (rv == 130) {
            printf("Experiment failed\n");
            exit(0);
        }
        return rv;
    }

    *ciphertext = NULL;

    // Determine how much memory is required to store the ciphertext.
    rv = funcs->C_Encrypt(session, plaintext, plaintext_length, NULL, &(*ciphertext_length));

    // The ciphertext will be prepended with the HSM generated IV
    // so the length must include the IV
    *ciphertext_length += AES_GCM_IV_SIZE;
    if (CKR_OK != rv) {
        printf("Failed to find GCM ciphertext length\n");
        goto done;
    }

    // Allocate memory to store the ciphertext.
    *ciphertext = (CK_BYTE_PTR)malloc(*ciphertext_length);
    if (NULL == *ciphertext) {
        rv = 1;
        printf("Failed to allocate ciphertext memory\n");
        goto done;
    }
    memset(*ciphertext, 0, *ciphertext_length);


    // Encrypt the data.
    rv = funcs->C_Encrypt(session, plaintext, plaintext_length, (*ciphertext) + AES_GCM_IV_SIZE, &(*ciphertext_length));


    // Prepend HSM generated IV to ciphertext buffer
    memcpy(*ciphertext, iv, AES_GCM_IV_SIZE);
    *ciphertext_length += AES_GCM_IV_SIZE;
    if (CKR_OK != rv) {
        printf("Encryption failed: %lu\n", rv);
        goto done;
    }


done:
    if (NULL != iv) {
        free(iv);
    }

    return rv;
}


CK_RV hsm_aes_decrypt(  CK_SESSION_HANDLE session, 
                        CK_OBJECT_HANDLE wrapping_key, 
                        CK_BYTE_PTR ciphertext, 
                        CK_ULONG ciphertext_length, 
                        CK_BYTE_PTR *decrypted_ciphertext, 
                        CK_ULONG *decrypted_ciphertext_length) {

    // Prepare the mechanism and return value
    CK_RV rv;
    CK_MECHANISM mech;

    // Additional Authenticated Data 
    CK_BYTE_PTR aad = (CK_BYTE_PTR)"plaintext aad";
    CK_ULONG aad_length = (CK_ULONG) strlen((char*)aad);

    // Setup the mechanism with the IV location and AAD information.
    CK_GCM_PARAMS params;
    params.ulIvLen = AES_GCM_IV_SIZE;
    params.ulIvBits = 0;
    params.pAAD = aad;
    params.ulAADLen = aad_length;
    params.ulTagBits = AES_GCM_TAG_SIZE * 8;

    mech.mechanism = CKM_AES_GCM;
    mech.ulParameterLen = sizeof(params);
    mech.pParameter = &params;

    //**********************************************************************************************
    // Decrypt
    //**********************************************************************************************  

    // Use the IV that was prepended -- The first AES_GCM_IV_SIZE bytes of the ciphertext.
    params.pIv = ciphertext;
    mech.ulParameterLen = sizeof(params);
    mech.pParameter = &params;

    rv = funcs->C_DecryptInit(session, &mech, wrapping_key);
    if (rv != CKR_OK) {
        printf("Decryption Init failed: %lu\n", rv);

        return rv;
    }
    CK_ULONG decrypted_ciphertext_length_temp;
    // Determine the length of decrypted ciphertext.
    rv = funcs->C_Decrypt(session, ciphertext + AES_GCM_IV_SIZE, ciphertext_length - AES_GCM_IV_SIZE,
                          NULL, &decrypted_ciphertext_length_temp);
    *decrypted_ciphertext_length = decrypted_ciphertext_length_temp;

    if (rv != CKR_OK) {
        printf("Decryption failed: %lu\n", rv);
        goto done;
    }

    // Allocate memory for the decrypted cipher text.
    *decrypted_ciphertext = (CK_BYTE_PTR)malloc(decrypted_ciphertext_length_temp + 1); //We want to null terminate the raw chars later
    if (NULL == *decrypted_ciphertext) {
        rv = 1;
        printf("Could not allocate memory for decrypted ciphertext\n");
        goto done;
    }

    // Decrypt the ciphertext.
    rv = funcs->C_Decrypt(session, ciphertext + AES_GCM_IV_SIZE, ciphertext_length - AES_GCM_IV_SIZE,
                          *decrypted_ciphertext, &decrypted_ciphertext_length_temp);
    if (rv != CKR_OK) {
        printf("Decrypted Ciphertext Length %d\n", *decrypted_ciphertext_length);
        printf("Decrypted Ciphertext Length %d\n", decrypted_ciphertext_length_temp);
        printf("Decryption failed: %lu\n", rv);
        goto done;
    }
    (*decrypted_ciphertext)[*decrypted_ciphertext_length] = 0; // Turn the chars into a C-String via null termination

    done:

    return rv;
}


/**
 * Generate an AES key with a template suitable for encrypting data.
 * The key is a Session key, and will be deleted once the HSM Session is closed.
 * @param session Active PKCS#11 session
 * @param key_length 16, 24, or 32 bytes
 * @param key Location where the key's handle will be written
 * @return CK_RV
 */
CK_RV generate_new_root_key(CK_SESSION_HANDLE session,
                       CK_ULONG key_length_bytes,
                       CK_OBJECT_HANDLE_PTR key) {
    CK_MECHANISM mech;

    mech.mechanism = CKM_AES_KEY_GEN;
    //mech.mechanism = CKM_GENERIC_SECRET_KEY_GEN;
    mech.ulParameterLen = 0;
    mech.pParameter = NULL;

    //printf("False_Val: %d\n", false_val);
    //printf("true_Val: %d\n", true_val);

    CK_ATTRIBUTE templ[] = {
            {CKA_TOKEN,       &true_val,            sizeof(CK_BBOOL)},
            {CKA_EXTRACTABLE, &true_val,             sizeof(CK_BBOOL)},
            {CKA_ENCRYPT,     &true_val,             sizeof(CK_BBOOL)},
            {CKA_DECRYPT,     &true_val,             sizeof(CK_BBOOL)},
            {CKA_VALUE_LEN,   &key_length_bytes, sizeof(CK_ULONG)},
    };

    CK_OBJECT_HANDLE test_key; 

    CK_RV rv = funcs->C_GenerateKey(session, &mech, templ, sizeof(templ) / sizeof(CK_ATTRIBUTE), key);
    // printf("RV: %d\n", rv);
    // printf("just key: %d\n", &test_key);
    // printf("key**: %d\n", test_key);
    return rv;
}


CK_RV generate_aes_key (   CK_SESSION_HANDLE session,
                                CK_ULONG key_length_bytes,
                                CK_OBJECT_HANDLE_PTR key) {
    // Open a Session to HSM
    CK_RV rv;
    int rc = EXIT_FAILURE;

    // Generate a new K-master Key
    CK_MECHANISM mech;

    mech.mechanism = CKM_AES_KEY_GEN;
    mech.ulParameterLen = 0;
    mech.pParameter = NULL;q

    CK_ATTRIBUTE templ[] = {
            {CKA_TOKEN,     &false_val,         sizeof(CK_BBOOL)},
            {CKA_VALUE_LEN, &key_length_bytes,  sizeof(key_length_bytes)},
            {CKA_ENCRYPT,   &true_val,  sizeof(CK_BBOOL)},
            {CKA_DECRYPT,   &true_val,  sizeof(CK_BBOOL)}
    };
    rv = funcs->C_GenerateKey(session, &mech, templ, sizeof(templ) / sizeof(CK_ATTRIBUTE), key);

    printf ("key 1 = %d\n", *key);

    return rv;
}


CK_RV delete_old_k_master(  CK_SESSION_HANDLE session,
                            CK_OBJECT_HANDLE old_key) {
    CK_RV rv;
    
    rv = funcs->C_DestroyObject(session, old_key);

    return rv;
}
