#ifndef EXAMPLE_BOUNDED_UPDATES_H
#define EXAMPLE_BOUNDED_UPDATES_H

#include "../ue_interface.h"

#include <string.h>
#include <stdio.h>

class example_class : public ue {
	public:	
		int keygen (struct key* key_ret) { printf("example keygen\n"); return 0; }
    		int enc (struct ctxt* ctxt_ret, struct key* key_ptr, struct ptxt* ptxt_ptr) { printf ("example enc\n"); return 0; }	
		int dec (struct ptxt* ptxt_ret, struct key* key_ptr, struct ctxt* ctxt_ptr) { printf ("example dec\n"); return 0; }
		int prepare_tokgen (struct prep_tokgen_download* prep_ret) { printf ("example prepare_tokgen\n"); return 0; }
		int tokgen (struct token* token_ret, struct key* key_ptr_1, struct key* key_ptr_2, struct tokgen_download* tokgen_download_ptr) { printf("example tokgen\n"); return 0; }
		int upd (struct ctxt* ctxt_ret, struct token* token_ptr, struct ctxt* ctxt_ptr) { printf("example upd\n"); return 0; }
};



#endif
