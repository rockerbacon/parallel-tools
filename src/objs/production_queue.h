#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

#include <iostream>

namespace parallel_tools {
	template<typename resource_type>
	class production_queue {
		private:
			std::queue<resource_type> producers_queue;
			std::queue<resource_type> consumers_queue;
			std::mutex producers_mutex;
			std::mutex consumers_mutex;
			std::condition_variable consumer_notifier;
			std::atomic<size_t> available_resources;
			std::atomic<size_t> unpublished_resources;
			std::atomic<size_t> waiting_consumers;
			const size_t maximum_task_delay;
			std::atomic<bool> swap_in_progress;

		public:
			production_queue(size_t maximum_task_delay = 1) :
				available_resources(0),
				unpublished_resources(0),
				waiting_consumers(0),
				maximum_task_delay(maximum_task_delay),
				swap_in_progress(false)
			{}

			void flush_production() {
				if (!swap_in_progress) {
					bool swapped_queues = false;
					swap_in_progress = true;
					{
						std::scoped_lock lock(consumers_mutex, producers_mutex);
						if (unpublished_resources > 0) {
							std::swap(producers_queue, consumers_queue);
							available_resources = consumers_queue.size();
							unpublished_resources = producers_queue.size();
							swapped_queues = true;
						}
					}
					swap_in_progress = false;
					if (swapped_queues) {
						consumer_notifier.notify_all();
					}
				}
			}

			template<typename... args_types>
			void produce(args_types... constructor_args) {
				resource_type resource(std::forward<args_types...>(constructor_args...));
				{
					std::lock_guard lock(producers_mutex);
					producers_queue.emplace(std::move(resource));
					unpublished_resources++;
				}
				if (available_resources == 0 && unpublished_resources >= maximum_task_delay) {
					flush_production();
				}
			}

			resource_type consume () {
				if (available_resources == 0 && unpublished_resources > 0) {
					flush_production();
				}
				resource_type resource;
				{
					std::unique_lock lock(consumers_mutex);
					waiting_consumers++;
					consumer_notifier.wait(lock, [&] { return available_resources > 0; });
					waiting_consumers--;

					resource = std::move(consumers_queue.front());
					consumers_queue.pop();
					available_resources--;
				}
				return resource;
			}

			size_t get_available_resources() {
				return available_resources;
			}

			size_t get_unpublished_resources() {
				return unpublished_resources;
			}
	};
}

