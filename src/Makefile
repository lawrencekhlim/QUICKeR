CC = /usr/bin/gcc
C2 = /usr/bin/g++
NISTFLAGS = -O3 -fomit-frame-pointer -march=native -fPIC -no-pie


UTILS_CPP = utils.cpp utils.h

UTILS_DIR = ue_schemes/utils
UTILS_DEPS = $(UTILS_DIR)/aes_gcm.h $(UTILS_DIR)/aes_gcm.c
#UTILS_O = $(UTILS_DIR)/aes_gcm.o

UAE_DIR = ue_schemes/uae_bounded_updates
UAE_DEPS = $(UAE_DIR)/aes_ctr.c $(UAE_DIR)/aes_ctr.h $(UAE_DIR)/AE_Nested_AES.c $(UAE_DIR)/AE_Nested_AES.h
#UAE_O = $(UAE_DIR)/aes_ctr.o $(UAE_DIR)/aes_ctr.h $(UAE_DIR)/AE_Nested_AES.o $(UAE_DIR)/AE_Nested_AES.h

ACTION_DIR = actions
ACTION_DEPS = $(ACTION_DIR)/actions.h $(ACTION_DIR)/actions.cpp $(ACTION_DIR)/client_actions.h $(ACTION_DIR)/client_actions.cpp

INTERFACE = ue_interface.cpp

HSM_FUNCTIONS_DIR = hsm_functions
COMMON_C = $(HSM_FUNCTIONS_DIR)/common.c
COMMON_O = $(HSM_FUNCTIONS_DIR)/common.o
GOPT_C = $(HSM_FUNCTIONS_DIR)/gopt.c
GOPT_O = $(HSM_FUNCTIONS_DIR)/gopt.o
HSM_FUNCTIONS_CPP = $(HSM_FUNCTIONS_DIR)/hsm_functions.cpp
HSM_FUNCTIONS_O = $(HSM_FUNCTIONS_DIR)/hsm_functions.o
HSM_FUNCTIONS_COMPILE = $(COMMON_O) $(GOPT_O)
HSM_FUNCTIONS_DEPS = $(HSM_FUNCTIONS_COMPILE) $(HSM_FUNCTIONS_O)
LIBCLOUDHSM = $(HSM_FUNCTIONS_DIR)/libcloudhsmpkcs11.a
HSM_FUNCTIONS_FLAGS = -rdynamic $(LIBCLOUDHSM)


QUEUE_DIR = update_queue
QUEUE_DEPS = $(QUEUE_DIR)/update_queue.cpp $(QUEUE_DIR)/update_queue.h

all:	TEST BENCH SERVER CLIENT UPDATECLIENT QUICKERSERVER QUICKERCLIENT QUICKERUPDATECLIENT QUICKERINIT

QUICKER:	QUICKERSERVER QUICKERCLIENT QUICKERUPDATECLIENT QUICKERINIT


$(COMMON_O): $(COMMON_C)
	$(CC) $(NISTFLAGS) -c $(COMMON_C) -o $(COMMON_O) 

$(GOPT_O): $(GOPT_C)
	$(CC) $(NISTFLAGS) -c $(GOPT_C) -o $(GOPT_O)

$(HSM_FUNCTIONS_O): $(HSM_FUNCTIONS_CPP) $(COMMON_O) $(GOPT_O) $(LIBCLOUDHSM)
	$(C2) $(NISTFLAGS) -o $(HSM_FUNCTIONS_O) -c $(HSM_FUNCTIONS_CPP) $(COMMON_O) $(GOPT_O) $(HSM_FUNCTIONS_FLAGS) -ldl 


TEST: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE)
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS) $(INTERFACE) -msse2avx -mavx2 ue_interface_test.cpp -o ue_interface $(RECRYPT_LIB) -lcrypto -g -pthread -ldl

BENCH: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE)
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS) $(INTERFACE) -msse2avx -mavx2 microbenchmark.cpp -o bench $(RECRYPT_LIB) -lcrypto -g -pthread -ldl

SERVER: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE)
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS) $(INTERFACE) -msse2avx -mavx2 server.cpp -o server $(RECRYPT_LIB) -lcrypto -g -pthread -ldl


CLIENT: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE)
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS) $(INTERFACE)  -msse2avx -mavx2 client.cpp -o client -lcrypto -g -pthread -ldl

UPDATECLIENT: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE) 
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS) $(INTERFACE)  -msse2avx -mavx2 update_client.cpp -o update_client -lcrypto -g -pthread -ldl


QUICKERSERVER: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE) $(HSM_FUNCTIONS_DEPS)
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS) $(LWE_UAE_DEPS) $(SHINE_DEPS) $(SHINE_FLAGS) $(RECRYPT_DEPS) $(ACTION_DEPS) $(INTERFACE) $(HSM_FUNCTIONS_DEPS)  -msse2avx -mavx2 quicker_server.cpp -o quicker_server $(HSM_FUNCTIONS_FLAGS) -lcrypto -g -pthread -ldl


QUICKERCLIENT: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE) $(HSM_FUNCTIONS_DEPS) $(QUEUE_DEPS)
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS)  $(ACTION_DEPS) $(INTERFACE) $(HSM_FUNCTIONS_DEPS) $(QUEUE_DEPS)  -msse2avx -mavx2 quicker_client.cpp -o quicker_client $(HSM_FUNCTIONS_FLAGS) -lcrypto -g -pthread -ldl

QUICKERUPDATECLIENT: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE) $(HSM_FUNCTIONS_DEPS) $(QUEUE_DEPS)
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS) $(ACTION_DEPS) $(INTERFACE) $(HSM_FUNCTIONS_DEPS) $(QUEUE_DEPS)  -msse2avx -mavx2 quicker_update_client.cpp -o quicker_update_client $(HSM_FUNCTIONS_FLAGS) -lcrypto -g -pthread -ldl

QUICKERINIT: $(UTILS_DEPS) $(UTILS_CPP) $(UAE_DEPS) $(INTERFACE) $(HSM_FUNCTIONS_DEPS)
	$(C2) $(NISTFLAGS) -DMODULO=602048 $(UTILS_CPP) $(UTILS_DEPS) $(UAE_DEPS) $(ACTION_DEPS) $(INTERFACE) $(HSM_FUNCTIONS_DEPS)  -msse2avx -mavx2 quicker_init.cpp -o quicker_init $(HSM_FUNCTIONS_FLAGS) -lcrypto -g -pthread -ldl




.PHONY: clean

clean:
	-rm ue_interface
	-rm server
	-rm client
	-rm update_client
	-rm bench
	-rm quicker_server
	-rm quicker_client
	-rm quicker_update_client
	-rm quicker_init
	-rm $(COMMON_O)
	-rm $(GOPT_O)
	-rm $(HSM_FUNCTIONS_O)
