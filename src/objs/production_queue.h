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
			std::atomic<size_t> idle_consumers;
			const size_t assured_consumers;
			std::atomic<bool> swap_in_progress;

			void swap_queues_if_needed(size_t min_idle_consumers) {
				bool swaped_queues = false;
				if (available_consumer_resources == 0 && idle_consumers >= min_idle_consumers && !swap_in_progress) {
					swap_in_progress = true;
					std::scoped_lock lock(consumers_mutex, producers_mutex);
					if (available_consumer_resources == 0 && idle_consumers >= min_idle_consumers && producers_queue.size() > 0) {
						std::swap(producers_queue, consumers_queue);
						available_consumer_resources = consumers_queue.size();
						swaped_queues = true;
					}
				}
				if (swaped_queues) {
					consumer_notifier.notify_all();
				}
				swap_in_progress = false;
			}
		public:
			production_queue(size_t assured_consumers = 1) :
				available_consumer_resources(0),
				idle_consumers(0),
				assured_consumers(assured_consumers),
				swap_in_progress(false)
			{}

			template<typename... args_types>
			void produce(args_types... constructor_args) {
				resource_type resource(std::forward<args_types...>(constructor_args...));
				{
					std::lock_guard lock(producers_mutex);
					producers_queue.emplace(std::move(resource));
				}
				swap_queues_if_needed(assured_consumers);
			}

			resource_type consume () {
				idle_consumers++;
				resource_type resource;
				{
					std::unique_lock lock(consumers_mutex);
					consumer_notifier.wait(lock, [&] { return available_consumer_resources > 0; });

					resource = std::move(consumers_queue.front());
					consumers_queue.pop();
					available_consumer_resources--;
				}
				idle_consumers--;
				swap_queues_if_needed(0);
				return resource;
			}

			size_t readily_available_resources() {
				return available_consumer_resources;
			}

			size_t unpublished_resources() {
				std::lock_guard lock(producers_mutex);
				return producers_queue.size();
			}
	};
}

