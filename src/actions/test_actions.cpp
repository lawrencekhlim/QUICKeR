#include "actions.h"
#include "../hsm_functions/hsm_functions.h"
#include "../ue_interface.h"


int main (int argc, char** argv) {
	ue* scheme = updatable_encryption_function ("naive");
	
	const char* val = "hello there!";
	struct ptxt p;
	p.ptxtptr = (void*)val;
	p.len = strlen(val) + 1;

	struct ctxt c;
	struct wrap w;

	int num_threads = 2;

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
	
	// open all sessions
  	open_sessions(sessions, num_threads);

	// only need to login once for all sessions
  	printf("session1: Logging in to slot via session1.\n");
  	CK_RV rv = funcs->C_Login((sessions[0]), CKU_USER, (CK_UTF8CHAR_PTR)args.pin, (CK_ULONG)strlen(args.pin));
  	if (rv != CKR_OK) {
    		fprintf(stderr, "session1: Failed to login.\n");
    		return rv;
  	}

	const char* text = "This is a big test!";
	CK_ULONG length = (CK_ULONG)((CK_ULONG)strlen (text) + 1);

	printf ("Generating key\n");
	CK_OBJECT_HANDLE key1;
	rv = generate_aes_key((sessions[0]), 16, &key1);

	printf ("Encrypting\n");
	int i = encrypt_and_upload((sessions[0]), key1, &w, &c, &p, scheme);
	
	struct ptxt p2;

	printf ("Decrypting\n");
	i = download_and_decrypt ((sessions[0]), key1, &p2, &w, &c, scheme); 
	printf ("Got back: %s\n", p2.ptxtptr);
	

	printf ("Generating key2\n");
	CK_OBJECT_HANDLE key2;
	rv = generate_aes_key((sessions[0]), 16, &key2);
	printf ("Updating!\n");

	struct wrap wrap2;
	struct token t;
	
    	struct prep_tokgen_download ptd;
	scheme->prepare_tokgen (&ptd);
    	printf ("ptd.end_index  = %d\n", ptd.end_index);
    	printf ("ptd.start_index = %d\n", ptd.start_index);

	struct tokgen_download td;
    	td.downloadptr = (void*) (((char*)c.ctxtptr) + ptd.start_index);
    	td.len = ptd.end_index - ptd.start_index;
    	if (ptd.end_index == -1 && ptd.start_index == -1) {
		td.downloadptr = c.ctxtptr;
        	td.len = c.len;
    	}

	i = perform_update (sessions[1], key1, key2, &wrap2, &t, &w, &td, scheme); 

	struct ctxt c2;
	scheme->upd(&c2, &t, &c);
	
	printf ("Decrypting!");
	struct ptxt p3;
	i = download_and_decrypt ((sessions[0]), key2, &p3, &wrap2, &c2, scheme); 
	printf ("Got back: %s\n", p3.ptxtptr);
}
