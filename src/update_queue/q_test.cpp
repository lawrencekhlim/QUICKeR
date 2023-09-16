#include "update_queue.h"
#include <pthread.h>
#include <unistd.h>

void * func (void* args) {
	while (true) {
		std::string s = dequeue();
		printf ("consuming %s\n", s.c_str());
		sleep(2);	
		printf ("done consuming %s\n", s.c_str());
	}	
}


int main () {
	int num_threads = 3;
	std::string filename ("q_test.txt");
	
	init (num_threads);
	pthread_t * thread = (pthread_t*) malloc (sizeof(pthread_t) * num_threads);
	for (int i = 0; i < num_threads; i++) {
		printf ("Creating thread %d\n", i);
		int ret = pthread_create (&thread[i], NULL, &func, NULL);
	}

	while (true) {
		if (all_waiting()) {
			printf ("enqueue all\n");
			enqueue_all(filename);
		}
	}

}

