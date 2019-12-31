#include "thread_pool.h"

using namespace std;
using namespace parallel_tools;

thread_pool::thread_pool(unsigned number_of_threads) :
	running(true)
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
	long long tasks_for_wakeup = (long long)threads.size() - (long long)task_queue.available_resources();
	for (decltype(tasks_for_wakeup) i = 0; i < tasks_for_wakeup; i++) {
		exec([]{});
	}

	for (auto& thread : threads) {
		thread.join();
	}
}

bool thread_pool::is_running() const {
	return running;
}

