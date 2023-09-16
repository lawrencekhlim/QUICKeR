#ifndef ACTIONS_H
#define ACTIONS_H

extern "C" {
	#include "../hsm_functions/common.h"
	#include "../hsm_functions/gopt.h"
}
#include "../hsm_functions/hsm_functions.h"
#include "../ue_interface.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


struct wrap {
	void* wrapptr;
	int len;
};

CK_RV encrypt_and_upload( CK_SESSION_HANDLE session, 
                        CK_OBJECT_HANDLE wrapping_key_handle, 
                        struct wrap* ret_wrap,
                        struct ctxt* ret_ctxt,
                        struct ptxt* ptxt, 
                        ue* scheme);


CK_RV download_and_decrypt (  CK_SESSION_HANDLE session, 
                            CK_OBJECT_HANDLE wrapping_key_handle,
                            struct ptxt* ret_ptxt, 
                            struct wrap* wrapped_key, 
                            struct ctxt* ctxt, 
                            ue* scheme);




int perform_update (    CK_SESSION_HANDLE session, 
                        CK_OBJECT_HANDLE wrapping_key_handle,
			CK_OBJECT_HANDLE new_wrapping_key_handle,
			struct wrap* ret_new_wrap,
                        struct token* ret_new_token, 
                        struct wrap* wrapped_old_key, 
                        struct tokgen_download* tdownload, 
                        ue* scheme);


#endif

