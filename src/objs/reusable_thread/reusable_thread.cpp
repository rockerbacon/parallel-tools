#include "../reusable_thread.h"

using namespace std;
using namespace parallel_tools;

reusable_thread::reusable_thread() :
	running(true),
	tasks_count(0),
	mutex(),
	notifier(),
	thread([this]() {
		while(running || tasks_count > 0) {
			packaged_task<void()> current_task;

			while (tasks_count == 0) { /* wait for tasks*/ }

			{
				unique_lock<std::mutex> lock(mutex);
				notifier.wait(lock, [this]{ return tasks_count > 0; });
				swap(current_task, task_queue.front());
				task_queue.pop();
				tasks_count--;
			}

			current_task();
		}
	})
{}

reusable_thread::~reusable_thread() {
	if (this->joinable()) {
		this->join();
	}
}

void reusable_thread::join() {
	this->running = false;
	this->exec([]{ });
	this->thread.join();
}

bool reusable_thread::joinable() const {
	return this->thread.joinable();
}

void reusable_thread::push_task(packaged_task<void()>&& packaged_task) {
	{
		lock_guard<std::mutex> lock(mutex);
		task_queue.emplace(move(packaged_task));
		tasks_count++;
	}
	notifier.notify_one();
}

future<void> reusable_thread::exec(const std::function<void()>& task) {
	packaged_task<void()> packaged_task(task);
	auto future = packaged_task.get_future();

	push_task(move(packaged_task));

	return future;
}

