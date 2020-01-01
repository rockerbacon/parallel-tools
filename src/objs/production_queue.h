#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <functional>

namespace parallel_tools {
	namespace flush_policy {
		struct batches_of { size_t batch_size; };
		struct maximum_waiting_consumers { size_t number_of_consumers; };
	}

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
			std::atomic<bool> swap_in_progress;
			std::function<bool()> flush_policy;

		public:
			production_queue() :
				available_resources(0),
				unpublished_resources(0),
				waiting_consumers(0),
				swap_in_progress(false),
				flush_policy([] { return true; })
			{}

			production_queue(const flush_policy::batches_of& batches) :
				production_queue()
			{
				flush_policy = [this, batches] {
					return unpublished_resources >= batches.batch_size;
				};
			}

			production_queue(const flush_policy::maximum_waiting_consumers& maximum_consumers) :
				production_queue()
			{
				flush_policy = [this, maximum_consumers] {
					return waiting_consumers >= maximum_consumers.number_of_consumers;
				};
			}

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
				if (available_resources == 0 && flush_policy()) {
					flush_production();
				}
			}

			resource_type consume () {
				waiting_consumers++;
				resource_type resource;
				{
					std::unique_lock lock(consumers_mutex);
					consumer_notifier.wait(lock, [&] { return available_resources > 0; });
					waiting_consumers--;

					resource = std::move(consumers_queue.front());
					consumers_queue.pop();
					available_resources--;
				}
				if (available_resources == 0 && unpublished_resources > 0) {
					flush_production();
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

