all:
	gcc -c common.c -o common.o -g
	gcc -c gopt.c -o gopt.o -g
	gcc -o hsm_functions.o -c hsm_functions.cpp common.o gopt.o -Werror -fpic -rdynamic libcloudhsmpkcs11.a -ldl -g 
	gcc -o hsm_test hsm_test.cpp hsm_functions.o gopt.o common.o -Werror -fpic -rdynamic libcloudhsmpkcs11.a -ldl -g 


