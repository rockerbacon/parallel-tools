#pragma once

#include <thread>
#include <mutex>
#include <future>
#include <queue>
#include <functional>

#include "production_queue.h"

namespace parallel_tools {

	class thread_pool {
		private:
			volatile bool running;
			production_queue<std::packaged_task<void()>> task_queue;
			std::vector<std::thread> threads;

			void init_threads(unsigned number_of_threads);

		public:
			thread_pool(unsigned number_of_threads);
			thread_pool(unsigned number_of_threads, const flush_policy::batches_of& batches);
			thread_pool(unsigned number_of_threads, const flush_policy::maximum_waiting_consumers& waiting_threads);
			~thread_pool();

			void terminate();
			bool is_running() const;
			void complete_batch();

			template<
				typename function_type,
				typename... args_types,
				typename return_type = typename std::result_of<function_type(args_types...)>::type
			>
			std::future<return_type> exec(const function_type& task, args_types... args) {
				std::packaged_task<return_type()> packaged_task(std::bind(task, args...));
				auto future = packaged_task.get_future();

				task_queue.produce(std::move(packaged_task));

				return future;
			}

			template<
				typename function_type,
				typename return_type = typename std::result_of<function_type()>::type
			>
			std::future<return_type> exec(const function_type& task) {
				std::packaged_task<return_type()> packaged_task(task);
				auto future = packaged_task.get_future();

				task_queue.produce(std::move(packaged_task));

				return future;
			}

	};

}
