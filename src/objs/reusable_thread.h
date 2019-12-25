#pragma once

#include <thread>
#include <mutex>
#include <future>
#include <queue>
#include <functional>

#include "compound_future.h"

namespace parallel_tools {

	class reusable_thread {
		private:
			volatile bool running;
			volatile unsigned tasks_count;
			std::mutex mutex;
			std::condition_variable notifier;
			std::thread thread;
			std::queue<std::packaged_task<void()>> task_queue;

			void push_task(std::packaged_task<void()>&& packaged_task);

		public:
			reusable_thread();
			~reusable_thread();

			void join();
			bool joinable() const;

			std::future<void> exec(const std::function<void()>& task);

			template<
				typename function_type,
				typename... args_types,
				typename return_type = typename std::result_of<function_type(args_types...)>::type
			>
			typename std::enable_if<
				!std::is_same<return_type, void>::value,
				compound_future<return_type>
			>::type
			exec(const function_type& task, args_types... args) {
				std::shared_ptr<return_type> return_value(new return_type);

				std::packaged_task<void()> packaged_task([
					task = std::bind(task, args...),
					return_value
			   	] () {
					*return_value = task();
				});

				compound_future<return_type> future(return_value, packaged_task.get_future());

				push_task(std::move(packaged_task));

				return std::move(future);
			}

			template<
				typename function_type,
				typename... args_types,
				typename return_type = typename std::result_of<function_type(args_types...)>::type
			>
			typename std::enable_if<
				std::is_same<return_type, void>::value,
				std::future<return_type>
			>::type
			exec(const function_type& task, args_types... args) {
				std::packaged_task<void()> packaged_task(std::bind(task, args...));
				auto future = packaged_task.get_future();

				push_task(std::move(packaged_task));

				return future;
			}
	};

}
