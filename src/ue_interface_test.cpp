#include <stdlib.h>
#include "ue_interface.h"
#include <string.h>
#include <stdio.h>


int backup_plaintext_size = 100;

int main (int argc, char** argv) {
    if (argc < 3) {
	printf ("Usage: ./ue_interface <ue scheme> <ptxt size>\n");
	exit (0);
    }

    char* tmpptr;
    int ptxt_size = strtol (argv[2], &tmpptr, 10);


    struct key* key1 = (struct key*) malloc (sizeof(struct key));
    struct ctxt* ctxt1 = (struct ctxt*) malloc(sizeof(struct ctxt));

    struct ptxt* ptxt1 = (struct ptxt*) malloc(sizeof(struct ptxt));
    char* plaintext = (char*) malloc (ptxt_size + 1);
    memset (plaintext, '0', ptxt_size); 
    plaintext[ptxt_size-1] = '\0';
    ptxt1->ptxtptr = plaintext;
    ptxt1->len = ptxt_size; 

    struct token* token1 = (struct token*) malloc(sizeof(struct token));


    printf ("----- Encrypting plaintext = %s\n", ptxt1->ptxtptr);
    /// TESTING COMMANDS
    ue* scheme1 = updatable_encryption_function(argv[1]);
    scheme1->keygen(key1);
    scheme1->enc(ctxt1, key1, ptxt1);

    printf ("----- Decrypting\n");
    scheme1->dec(ptxt1, key1, ctxt1);
    printf ("----- Got back: %.*s\n", ptxt1->len, ptxt1->ptxtptr);

    // setting up new token
    struct key* key2 = (struct key*) malloc (sizeof(struct key));
    scheme1->keygen(key2);

    // what to download
    struct prep_tokgen_download ptd;
    scheme1->prepare_tokgen (&ptd);
    printf ("ptd.end_index  = %d\n", ptd.end_index);
    printf ("ptd.start_index = %d\n", ptd.start_index);

    struct tokgen_download* tdownload = (struct tokgen_download*) malloc (sizeof (struct tokgen_download));
    tdownload->downloadptr = ctxt1->ctxtptr + ptd.start_index;
    tdownload->len = ptd.end_index - ptd.start_index;
    if (ptd.end_index == -1 && ptd.start_index == -1) {
	tdownload->downloadptr = ctxt1->ctxtptr;
        tdownload->len = ctxt1->len;
    }
    
    printf ("----- Before tokgen\n");
    // tokgen
    scheme1->tokgen(token1, key1, key2, tdownload);
    
    printf ("----- Before upd\n");
    // update
    struct ctxt* ctxt2 = (struct ctxt*) malloc(sizeof(struct ctxt));
    scheme1->upd(ctxt2, token1, ctxt1);
   
    printf ("----- Decrypting\n"); 
    // decrypt update
    scheme1->dec(ptxt1, key2, ctxt2);
    printf ("----- Got back: %s\n", ptxt1->ptxtptr);


    ue* scheme2 = updatable_encryption_function(argv[1]);
    // Try again test
    //
    // setting up new token
    scheme1->keygen(key2);

    // what to download
    //struct prep_tokgen_download ptd;
    scheme1->prepare_tokgen (&ptd);

    tdownload->downloadptr = ctxt1->ctxtptr + ptd.start_index;
    tdownload->len = ptd.end_index - ptd.start_index;
    if (ptd.end_index == -1 && ptd.start_index == -1) {
	tdownload->downloadptr = ctxt1->ctxtptr;
        tdownload->len = ctxt1->len;
    }
    
    printf ("----- Before tokgen\n");
    // tokgen
    scheme1->tokgen(token1, key1, key2, tdownload);
    
    printf ("----- Before upd\n");
    // update
    scheme2->upd(ctxt2, token1, ctxt1);
   
    printf ("----- Decrypting\n"); 
    // decrypt update
    scheme1->dec(ptxt1, key2, ctxt2);
    printf ("----- Got back: %s\n", ptxt1->ptxtptr);

    // Stop here

    printf ("----- End Test\n");

}
