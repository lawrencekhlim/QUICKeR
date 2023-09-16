

UTILS_DIR = ../ue_schemes/utils
UTILS_DEPS = $(UTILS_DIR)/aes_gcm.h $(UTILS_DIR)/aes_gcm.c

UAE_DIR = ../ue_schemes/uae_bounded_updates
UAE_DEPS = $(UAE_DIR)/aes_ctr.c $(UAE_DIR)/aes_ctr.h $(UAE_DIR)/AE_Nested_AES.c

all:
	gcc -c ../hsm_functions/common.c -o ../hsm_functions/common.o
	gcc -c ../hsm_functions/gopt.c -o ../hsm_functions/gopt.o
	g++ -o ../hsm_functions/hsm_functions.o -c ../hsm_functions/hsm_functions.cpp ../hsm_functions/common.o ../hsm_functions/gopt.o -Werror -fpic -rdynamic ../hsm_functions/libcloudhsmpkcs11.a -ldl 
	g++ -o actions.o -c actions.cpp ../hsm_functions/hsm_functions.o ../hsm_functions/common.o ../hsm_functions/gopt.o -Werror -fpic -rdynamic ../hsm_functions/libcloudhsmpkcs11.a -ldl 
	g++ -o test_actions test_actions.cpp ../ue_interface.cpp $(UAE_DEPS) $(UTILS_DEPS)  actions.o ../hsm_functions/hsm_functions.o ../hsm_functions/common.o ../hsm_functions/gopt.o -Werror -fpic -rdynamic ../hsm_functions/libcloudhsmpkcs11.a -ldl -lssl -lcrypto 
