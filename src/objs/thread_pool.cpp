#include "thread_pool.h"

using namespace std;
using namespace parallel_tools;

void thread_pool::init_threads(unsigned number_of_threads) {
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

thread_pool::thread_pool(unsigned number_of_threads) :
	running(true),
	task_queue(number_of_threads)
{
	init_threads(number_of_threads);
}

thread_pool::thread_pool(unsigned number_of_threads, const flush_policy::batches_of& batches) :
	running(true),
	task_queue(number_of_threads, batches)
{
	init_threads(number_of_threads);
}

thread_pool::thread_pool(unsigned number_of_threads, const flush_policy::maximum_waiting_consumers& waiting_threads) :
	running(true),
	task_queue(number_of_threads, waiting_threads)
{
	init_threads(number_of_threads);
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

