#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

namespace parallel_tools {
	template<typename resource_type>
	class production_queue {
		private:
			std::queue<resource_type> producers_queue;
			std::queue<resource_type> consumers_queue;
			std::mutex producers_mutex;
			std::mutex consumers_mutex;
			std::condition_variable consumer_notifier;
			std::atomic<size_t> available_consumer_resources;
			std::atomic<size_t> unpublished_resources;
			std::atomic<size_t> waiting_consumers;
			const size_t maximum_initial_task_delay;
			std::atomic<bool> swap_in_progress;

			void swap_queues() {
				if (!swap_in_progress) {
					swap_in_progress = true;
					{
						std::scoped_lock lock(consumers_mutex, producers_mutex);
						std::swap(producers_queue, consumers_queue);
						available_consumer_resources = consumers_queue.size();
						unpublished_resources = 0;
					}
					consumer_notifier.notify_all();
					swap_in_progress = false;
				}
			}
		public:
			production_queue(size_t maximum_initial_task_delay = 1) :
				available_consumer_resources(0),
				unpublished_resources(0),
				waiting_consumers(0),
				maximum_initial_task_delay(maximum_initial_task_delay),
				swap_in_progress(false)
			{}

			template<typename... args_types>
			void produce(args_types... constructor_args) {
				resource_type resource(std::forward<args_types...>(constructor_args...));
				{
					std::lock_guard lock(producers_mutex);
					producers_queue.emplace(std::move(resource));
					unpublished_resources++;
				}
				if (available_consumer_resources == 0 && unpublished_resources >= maximum_initial_task_delay) {
					swap_queues();
				}
			}

			resource_type consume () {
				if (available_consumer_resources == 0 && unpublished_resources > 0) {
					swap_queues();
				}
				resource_type resource;
				{
					std::unique_lock lock(consumers_mutex);
					waiting_consumers++;
					consumer_notifier.wait(lock, [&] { return available_consumer_resources > 0; });
					waiting_consumers--;

					resource = std::move(consumers_queue.front());
					consumers_queue.pop();
					available_consumer_resources--;
				}
				return resource;
			}
	};
}

