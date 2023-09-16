#include <stdlib.h>
#include "ue_interface.h"

#include <string.h>
#include <stdio.h>
#include <time.h>


int backup_plaintext_size = 100;


int main (int argc, char** argv) {
    if (argc < 3) {
	printf ("Usage: ./bench <ue scheme> <ptxt size>\n");
	exit (0);
    }

    char* tmpptr;
    int ptxt_size = strtol (argv[2], &tmpptr, 10);
    int loops = 100;
   

    double gen_cycles = 0;
    double encrypt_cycles = 0;
    double regen_cycles = 0;
    double re_encrypt_cycles = 0;
    double decrypt_cycles = 0;


    clock_t begin;
    clock_t end;

    struct key* key1 = (struct key*) malloc (sizeof(struct key));
    struct ctxt* ctxt1 = (struct ctxt*) malloc(sizeof(struct ctxt));

    struct ptxt* ptxt1 = (struct ptxt*) malloc(sizeof(struct ptxt));
    char* plaintext = (char*) malloc (ptxt_size);
    memset (plaintext, 'a', ptxt_size);
    ptxt1->ptxtptr = plaintext;
    ptxt1->len = ptxt_size; 

    struct token* token1 = (struct token*) malloc(sizeof(struct token));


    /// TESTING COMMANDS
    ue* scheme1 = updatable_encryption_function(argv[1]);
    for (int i = 0; i < loops; i++) {
    	begin = clock();
	scheme1->keygen(key1);
        end = clock();
        gen_cycles += (double)(end - begin);
	
	begin = clock();	
	scheme1->enc(ctxt1, key1, ptxt1);
    	end = clock();
	encrypt_cycles += (double) (end - begin);
	if (loops - 1 != i) {
	    free (key1->keyptr);
	    free (ctxt1->ctxtptr);
	}
    }

    //printf ("----- Testing Update and Decryption\n");

    struct key* key2 = (struct key*) malloc (sizeof(struct key));
    struct prep_tokgen_download ptd;
    struct tokgen_download* tdownload = (struct tokgen_download*) malloc (sizeof (struct tokgen_download));
    struct ctxt* ctxt2 = (struct ctxt*) malloc(sizeof(struct ctxt));
    
    for (int i = 0; i < loops; i++) {
        scheme1->increment_num_updates();
    
        scheme1->keygen(key2);

        // what to download
        scheme1->prepare_tokgen (&ptd);

        tdownload->downloadptr = ctxt1->ctxtptr;
        tdownload->len = ptd.end_index - ptd.start_index;
        if (ptd.end_index == -1 && ptd.start_index == -1) {
            tdownload->len = ctxt1->len;
        }
    
        // tokgen
        begin = clock();
        scheme1->tokgen(token1, key1, key2, tdownload);
        end = clock();
        regen_cycles += (double) (end - begin);

        // update
        begin = clock();
        scheme1->upd(ctxt2, token1, ctxt1);
        end = clock();
        re_encrypt_cycles += (double) (end - begin);

        // decrypt update
        begin = clock();
        scheme1->dec(ptxt1, key2, ctxt2);
    	end = clock();
        decrypt_cycles += (double) (end - begin);

        free (key1->keyptr);
        free (ctxt1->ctxtptr);
        free (token1->tokptr);
        
        key1->keyptr = key2->keyptr;
        key1->len = key2->len;

        ctxt1->ctxtptr = ctxt2->ctxtptr;
        ctxt1->len = ctxt2->len;
    }

    gen_cycles /= CLOCKS_PER_SEC;
    encrypt_cycles /= CLOCKS_PER_SEC;
    regen_cycles /= CLOCKS_PER_SEC;
    re_encrypt_cycles /= CLOCKS_PER_SEC;
    decrypt_cycles /=CLOCKS_PER_SEC;

    printf("UE Scheme: %s\n", argv[1]);
    printf("  Size:%d  Runs:%u\n gen_key:    %f %d\n encrypt:    %f %d\n regen_key:  %f %d\n re_encrypt: %f %d\n decrypt:    %f %d\n\n",
        ptxt_size, loops,
        gen_cycles, loops,
        encrypt_cycles, loops,
        regen_cycles, loops,
        re_encrypt_cycles, loops,
        decrypt_cycles, loops);

}
