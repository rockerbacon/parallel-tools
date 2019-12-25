#include "thread_pool.h"

using namespace std;
using namespace parallel_tools;

thread_pool::thread_pool(unsigned number_of_threads) :
	running(true),
	tasks_count(0),
	task_queue_mutex(),
	thread_notifier()
{
	threads.reserve(number_of_threads);
	for (decltype(number_of_threads) i = 0; i < number_of_threads; i++) {
		threads.emplace_back([this] {
			while(running) {
				packaged_task<void()> current_task;

				{
					unique_lock<std::mutex> lock(task_queue_mutex);
					thread_notifier.wait(lock, [this]{ return tasks_count > 0; });
					swap(current_task, task_queue.front());
					task_queue.pop();
					tasks_count--;
				}

				current_task();
			}
		});
	}
}

thread_pool::~thread_pool() {
	if (joinable()) {
		join();
	}
}

void thread_pool::join() {
	running = false;
	for (unsigned i = 0; i < threads.size(); i++) {
		exec([]{ });
	}

	for (auto& thread : threads) {
		thread.join();
	}
}

bool thread_pool::joinable() const {
	return running;
}

void thread_pool::push_task(packaged_task<void()>&& packaged_task) {
	{
		lock_guard<std::mutex> lock(mutex);
		task_queue.emplace(move(packaged_task));
		tasks_count++;
	}
	thread_notifier.notify_one();
}

future<void> thread_pool::exec(const std::function<void()>& task) {
	packaged_task<void()> packaged_task(task);
	auto future = packaged_task.get_future();

	push_task(move(packaged_task));

	return future;
}

