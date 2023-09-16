#include "update_queue.h"


pthread_mutex_t q_mtx;
pthread_cond_t is_empty_cond;
int is_empty = 0;
pthread_cond_t new_round;
bool need_new_round = 0;
pthread_mutex_t wait_lock_mtx;

int num_waiting = 0;
std::queue <std::string> q; 
int num_update_threads = 5;

int enqueue_all (std::string filename) {
	std::ifstream infile(filename);
	pthread_mutex_lock(&q_mtx);		
	std::string line;
	while (std::getline(infile, line))
	{
		q.push (trim(line));	
	}
	
	is_empty = num_update_threads;
	pthread_cond_broadcast(&is_empty_cond);
	pthread_mutex_unlock(&q_mtx);
}

std::string dequeue () {
	pthread_mutex_lock(&q_mtx);

	while (q.empty()) {	
		
		num_waiting += 1;
		if (num_waiting == num_update_threads) {
			need_new_round = 1;
			pthread_cond_signal(&new_round);
		}
		if (is_empty == 0) {
			pthread_cond_wait (&is_empty_cond, &q_mtx);
		}
		is_empty--;
		num_waiting -= 1;
	}

	std::string ret = q.front();
	q.pop();
	
	pthread_mutex_unlock(&q_mtx);
	return ret;
}

// maybe delete
int all_waiting () {
	pthread_mutex_lock (&q_mtx);
	if (need_new_round == 0) {
		pthread_cond_wait (&new_round, &q_mtx);
	}
	need_new_round = 0;
	pthread_mutex_unlock (&q_mtx);
	return 1;
}

void init (int num_threads) {
	num_update_threads = num_threads;
	pthread_cond_init (&is_empty_cond, NULL);
	pthread_cond_init (&new_round, NULL);
	pthread_mutex_init (&q_mtx, NULL);
}



std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}
 
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
 
std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}


