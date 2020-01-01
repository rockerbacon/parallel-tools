#include "thread_pool.h"

#include <iostream>

using namespace std;
using namespace parallel_tools;

thread_pool::thread_pool(unsigned number_of_threads, size_t maximum_task_delay) :
	running(true),
	task_queue(
		maximum_task_delay > 0 ? maximum_task_delay : number_of_threads,
		number_of_threads
	)
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

	for (auto& thread : threads) {
		thread.join();
	}
}

bool thread_pool::is_running() const {
	return running;
}

