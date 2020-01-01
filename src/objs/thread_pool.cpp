#include "thread_pool.h"

#include <iostream>

using namespace std;
using namespace parallel_tools;

#include <iostream>

thread_pool::thread_pool(unsigned number_of_threads, size_t maximum_batch_size) :
	running(true),
	task_queue(maximum_batch_size)
{
	threads.reserve(number_of_threads);
	for (decltype(number_of_threads) i = 0; i < number_of_threads; i++) {
		threads.emplace_back([this] {
			while(running) {
				auto current_task = task_queue.consume();
				current_task();
			}
		});
	}
}

thread_pool::~thread_pool() {
	if (is_running()) {
		terminate();
	}
}

void thread_pool::terminate() {
	running = false;
	for (size_t i = 0; i < threads.size(); i++) {
		exec([]{});
	}
	task_queue.flush_production();

	for (auto& thread : threads) {
		thread.join();
	}
}

bool thread_pool::is_running() const {
	return running;
}

void thread_pool::complete_batch() {
	task_queue.flush_production();
}

