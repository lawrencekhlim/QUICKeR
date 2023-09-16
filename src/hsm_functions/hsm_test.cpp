#include "hsm_functions.h"


int main (int argc, char** argv) {
	int num_threads = 1;

	CK_SESSION_HANDLE* sessions = (CK_SESSION_HANDLE*) malloc (num_threads * sizeof (CK_SESSION_HANDLE));
	
  	struct pkcs_arguments args = {0};
	
	// parse command line args for pkcs11 and login to HSM
  	if (get_pkcs_args(argc, argv, &args) < 0) {
    		return EXIT_FAILURE;
  	}

	printf ("args.library = %s\n", args.library);
	// Initialization of pkcs11 library
  	if (pkcs11_initialize(args.library) != CKR_OK) {
    		return EXIT_FAILURE;
  	}


  	CK_RV rv;
  	CK_SESSION_HANDLE my_session;


  	rv = pkcs11_open_session_2((CK_UTF8CHAR_PTR)(args.pin), &my_session);
    if (CKR_OK != rv) {
        printf("FAILED: pkcs11_open_session\n");
        return EXIT_FAILURE;
    }

	const char* text = "This is a big test!";
	CK_ULONG length = (CK_ULONG)((CK_ULONG)strlen (text) + 1);

	printf ("Generating key\n");
	CK_OBJECT_HANDLE key1 = CK_INVALID_HANDLE;
	rv = generate_aes_key(my_session, 16, &key1);
	printf ("key 1 = %lu\n", key1);


	CK_OBJECT_HANDLE key2 = CK_INVALID_HANDLE;
	rv = generate_aes_key(my_session, 16, &key2);
	printf ("key 2 = %lu\n", key2);

	printf ("Encrypting\n");
	CK_BYTE_PTR c1 = NULL;
	CK_ULONG c_len = 0;
	CK_SESSION_HANDLE sess = sessions[1];
	CK_BYTE_PTR ptxt = (CK_BYTE_PTR)text;
	CK_OBJECT_HANDLE test_key = 4611686018427387905;

	printf("size of key: %d\n", sizeof(CK_SESSION_HANDLE));
	printf("size of key: %d\n", sizeof(CK_ULONG));
	printf("size of key: %d\n", sizeof(test_key));
	if (key1 == test_key) {
		printf("EQUAL\n");
	} else {
		printf("NOT EQUAL\n");
	}
	printf("KEY1: %d\n", key1);
	printf("TESTKEY: %d\n", test_key);
	

	const char* aes_key = "iamtoogood";
    CK_BYTE_PTR wrapped_key = NULL;
    CK_ULONG wrapped_len = 0;
 	rv = hsm_aes_encrypt(my_session, key1, (CK_BYTE_PTR)aes_key, 10, &c1, &c_len);
 	printf("Encrypt rv: %d\n", rv);


	printf ("Decrypting\n");	
	CK_BYTE_PTR p1 = NULL;
	CK_ULONG p_len = 0;

	rv =  hsm_aes_decrypt(my_session, key1, c1, c_len, &p1, &p_len); 
	
	printf ("Got back: %s\n", (char*)p1);

}

