#include "ue_interface.h"
#include "ue_wrappers/uae_bounded_updates.h"
#include "ue_wrappers/wrapper_example.h"
#include "ue_wrappers/naive_updates.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>




ue* updatable_encryption_function (const char* arg) {
    ue* ret;
    if (strlen (arg) == strlen ("uae_bounded") && strncmp(arg, "uae_bounded", strlen ("uae_bounded")) == 0) {
        ret = new uae_bounded_updates(64);
    }
    else if (strlen(arg) == strlen ("naive") && strncmp (arg, "naive", strlen ("naive")) == 0) {
	   ret = new naive_updates(); 
    }
    else {
        // Something else
	   printf ("%s matches nothing! Exiting...\n", arg);
	   exit(0);
    }
    return ret;
}

