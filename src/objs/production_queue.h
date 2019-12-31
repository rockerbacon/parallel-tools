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
			std::queue<resource_type> queue;
			std::mutex queue_mutex;
			std::condition_variable consumer_notifier;
			volatile size_t resources_count;
		public:
			production_queue() :
				resources_count(0)
			{}

			template<typename... args_types>
			void produce(args_types... constructor_args) {
				{
					std::lock_guard lock(queue_mutex);
					queue.emplace(std::forward<args_types...>(constructor_args...));
					resources_count++;
				}
				consumer_notifier.notify_one();
			}

			resource_type consume () {
				std::unique_lock lock(queue_mutex);
				consumer_notifier.wait(lock, [&] { return resources_count > 0; });

				resource_type resource{std::move(queue.front())};
				queue.pop();
				resources_count--;
				return std::move(resource);
			}

			size_t available_resources() const {
				return resources_count;
			}
	};
}

