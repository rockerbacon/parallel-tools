#pragma once

#include <thread>
#include <mutex>
#include <future>
#include <queue>
#include <functional>

namespace parallel_tools {

	class reusable_thread {
		private:
			volatile bool running;
			volatile unsigned tasks_count;
			std::mutex mutex;
			std::condition_variable notifier;
			std::thread thread;
			std::queue<std::function<void()>> task_queue;

		public:
			reusable_thread();
			~reusable_thread();

			void join();
			bool joinable() const;

			template<
				typename function_type,
				typename... args_types,
				typename return_type = typename std::result_of<function_type(args_types...)>::type
			>
			std::future<return_type> exec(const function_type& task, args_types... args) {
				auto no_args_task = std::bind(task, args...);
				auto packaged_task = std::make_shared<std::packaged_task<return_type()>>(no_args_task);
				auto future = packaged_task->get_future();

				{
					std::lock_guard<std::mutex> lock(mutex);
					task_queue.push([ packaged_task ]() mutable {
						(*packaged_task)();
					});
					tasks_count++;
				}
				notifier.notify_one();

				return future;
			}
	};

	class thread_pile {
		private:
			unsigned number_of_threads;
			std::vector<reusable_thread> threads;
		public:
			typedef decltype(threads)::iterator iterator;
			struct slice_t {
				iterator begin;
				iterator end;

				slice_t(iterator begin, iterator end);

				slice_t slice(unsigned begin, unsigned end);
			};

			thread_pile(unsigned number_of_threads, unsigned call_depth);
			thread_pile(unsigned number_of_threads);
			thread_pile() = default;

			reusable_thread& operator[](unsigned thread_index);
			slice_t depth(unsigned depth);

			slice_t slice (unsigned begin, unsigned end);

			operator slice_t ();
	};

}
