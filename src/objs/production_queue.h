#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

#include <iostream>

namespace parallel_tools {
	template<typename resource_type>
	class production_queue {
		private:
			std::vector<resource_type> queue;
			size_t head;
			size_t tail;
			std::atomic<size_t> resources_count;
			std::condition_variable producer_notifier;
			std::condition_variable consumer_notifier;
			std::mutex producer_mutex;
			std::mutex consumer_mutex;
		public:
			production_queue(size_t size) :
				queue(size),
				head(0),
				tail(0),
				resources_count(0)
			{}

			void produce(const resource_type& resource) {
				{
					std::unique_lock lock(producer_mutex);
					producer_notifier.wait(lock, [&] { return resources_count < queue.size(); });

					queue[tail] = resource;
					tail = (tail+1) % queue.size();
					resources_count++;
				}
				consumer_notifier.notify_one();
			}

			resource_type consume () {
				resource_type resource;
				{
					std::unique_lock lock(consumer_mutex);
					consumer_notifier.wait(lock, [&] { return resources_count > 0; });

					resource = std::move(queue[head]);
					head = (head+1) % queue.size();
					resources_count--;
				}
				producer_notifier.notify_one();
				return std::move(resource);
			}

			size_t available_resources() const {
				return resources_count;
			}
	};
}

