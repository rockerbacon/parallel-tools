#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <functional>

namespace parallel_tools {
	namespace flush_policy {
		constexpr auto never = [] { return false; };
		constexpr auto always = [] { return true; };
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

			bool swap_queues() {
				bool swapped_queues = false;
				if (!swap_in_progress) {
					swap_in_progress = true;
					{
						std::lock_guard lock(producers_mutex);
						if (available_resources == 0 && unpublished_resources > 0) {
							std::swap(producers_queue, consumers_queue);
							available_resources = consumers_queue.size();
							unpublished_resources = 0;
							swapped_queues = true;
						}
					}
					swap_in_progress = false;
				}
				return swapped_queues;
			}

		public:
			template<typename function_type>
			typename std::enable_if<
				std::is_same<bool, typename std::result_of<function_type()>::type>::value,
				void
			>::type
			switch_policy(const function_type& custom_policy) {
				{
					std::lock_guard lock(consumers_mutex);
					flush_policy = custom_policy;
				}
				consumer_notifier.notify_one();
			}

			void switch_policy(const flush_policy::batches_of& batches) {
				{
					std::lock_guard lock(consumers_mutex);
					flush_policy = [this, batches] {
						return unpublished_resources >= batches.batch_size;
					};
				}
				consumer_notifier.notify_one();
			}

			void switch_policy(const flush_policy::maximum_waiting_consumers& maximum_consumers) {
				{
					std::lock_guard lock(consumers_mutex);
					flush_policy = [this, maximum_consumers] {
						return waiting_consumers > maximum_consumers.number_of_consumers;
					};
				}
				consumer_notifier.notify_one();
			}

			production_queue() :
				available_resources(0),
				unpublished_resources(0),
				waiting_consumers(0),
				swap_in_progress(false)
			{
				switch_policy(flush_policy::always);
			}

			template<typename function_type>
			production_queue(const function_type& custom_policy) :
				available_resources(0),
				unpublished_resources(0),
				waiting_consumers(0),
				swap_in_progress(false)
		   	{
				switch_policy(custom_policy);
			}

			production_queue(const flush_policy::batches_of& batches) :
				available_resources(0),
				unpublished_resources(0),
				waiting_consumers(0),
				swap_in_progress(false)
			{
				switch_policy(batches);
			}

			production_queue(const flush_policy::maximum_waiting_consumers& maximum_consumers) :
				available_resources(0),
				unpublished_resources(0),
				waiting_consumers(0),
				swap_in_progress(false)
			{
				switch_policy(maximum_consumers);
			}


			template<typename... args_types>
			void produce(args_types... constructor_args) {
				resource_type resource(std::forward<args_types...>(constructor_args...));
				{
					std::lock_guard lock(producers_mutex);
					producers_queue.emplace(std::move(resource));
					unpublished_resources++;
				}
				consumer_notifier.notify_one();
			}

			resource_type consume () {
				bool swapped_queues = false;
				waiting_consumers++;
				resource_type resource;
				{
					std::unique_lock lock(consumers_mutex);
					consumer_notifier.wait(lock, [&] {
						if (available_resources == 0 && unpublished_resources > 0 && flush_policy()) {
							swapped_queues = swap_queues();
						}
						return available_resources > 0;
					});
					waiting_consumers--;

					resource = std::move(consumers_queue.front());
					consumers_queue.pop();
					available_resources--;
				}
				if (swapped_queues) {
					consumer_notifier.notify_all();
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

