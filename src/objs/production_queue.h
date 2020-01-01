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

			void swap_queues_if_needed() {
				bool swaped_queues = false;
				if (available_consumer_resources == 0) {
					std::scoped_lock lock(consumers_mutex, producers_mutex);
					if (available_consumer_resources == 0 && !producers_queue.empty()) {
						std::swap(producers_queue, consumers_queue);
						available_consumer_resources = consumers_queue.size();
						swaped_queues = true;
					}
				}
				if (swaped_queues) {
					consumer_notifier.notify_all();
				}
			}
		public:
			production_queue() :
				available_consumer_resources(0)
			{}

			template<typename... args_types>
			void produce(args_types... constructor_args) {
				resource_type resource(std::forward<args_types...>(constructor_args...));
				{
					std::lock_guard lock(producers_mutex);
					producers_queue.emplace(std::move(resource));
				}
				swap_queues_if_needed();
			}

			resource_type consume () {
				resource_type resource;
				{
					std::unique_lock lock(consumers_mutex);
					consumer_notifier.wait(lock, [&] { return available_consumer_resources > 0; });

					resource = std::move(consumers_queue.front());
					consumers_queue.pop();
					available_consumer_resources--;
				}
				swap_queues_if_needed();
				return resource;
			}

			size_t available_resources() {
				std::lock_guard lock(producers_mutex);
				return available_consumer_resources+producers_queue.size();
			}
	};
}

