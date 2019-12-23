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
			function<void()> current_task;

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

thread_pile::slice_t::slice_t(iterator begin, iterator end) :
	begin(begin),
	end(end)
{}

thread_pile::slice_t thread_pile::slice_t::slice(unsigned begin, unsigned end) {
	return slice_t{
		this->begin+begin,
		this->begin+end
	};
}

thread_pile::thread_pile(unsigned number_of_threads, unsigned call_depth) :
	number_of_threads(number_of_threads),
	threads(number_of_threads*call_depth)
{}

thread_pile::thread_pile(unsigned number_of_threads) :
	thread_pile(number_of_threads, 1)
{}

reusable_thread& thread_pile::operator[](unsigned thread_index) {
	return this->threads[thread_index];
}

thread_pile::slice_t thread_pile::depth(unsigned depth) {
	auto depth_begin = this->threads.begin()+depth*this->number_of_threads;
	return slice_t{
		depth_begin,
		depth_begin+this->number_of_threads
	};
}

thread_pile::slice_t thread_pile::slice (unsigned begin, unsigned end) {
	return slice_t{
		this->threads.begin()+begin,
		this->threads.begin()+end
	};
}

thread_pile::operator thread_pile::slice_t () {
	return slice_t{
		this->threads.begin(),
		this->threads.begin()+this->number_of_threads
	};
}

