#ifndef UPDATE_QUEUE_H
#define UPDATE_QUEUE_H

#include <queue>
#include <string.h>
#include <fstream>
#include <sstream>
extern "C" {
	#include <pthread.h>
}
#include <algorithm>

extern pthread_mutex_t q_mtx;
extern pthread_cond_t is_empty_cond;
extern int is_empty;
extern pthread_cond_t new_round;
extern bool need_new_round;
extern pthread_mutex_t wait_lock_mtx;

extern int num_waiting;
extern std::queue <std::string> q; 
extern int num_update_threads;

const std::string WHITESPACE = " \n\r\t\f\v";

int enqueue_all (std::string filename);

std::string dequeue ();

int all_waiting ();

void init (int num_threads);


std::string ltrim(const std::string &s);
 
std::string rtrim(const std::string &s);
 
std::string trim(const std::string &s);


#endif
