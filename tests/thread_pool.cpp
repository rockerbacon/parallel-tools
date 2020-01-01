#include <assertions-test/test.h>
#include <thread_pool.h>
#include <stopwatch/stopwatch.h>

using namespace parallel_tools;
using namespace std;

begin_tests {
	test_suite("when using thread pools of only one thread") {
		test_case("pool should be able to execute more than one task") {
			thread_pool pool(1);
			int processed_value1 = 0;
			int processed_value2 = 0;

			auto future = pool.exec([&]() {
				processed_value1 = 10;
			});


			future = pool.exec([&]() {
				processed_value2 = 15;
			});

			future.wait();

			assert(processed_value1, ==, 10);
			assert(processed_value2, ==, 15);
		};

		test_case("pool should process functions with arguments and return") {
			thread_pool pool(1);
			auto subtraction = [](int arg1, int arg2) {
				return arg1 - arg2;
			};

			auto sum = [](int arg1, int arg2) -> int {
				return arg1 + arg2;
			};

			auto subtractionFuture = pool.exec(subtraction, 10, 2);
			auto sumFuture = pool.exec(sum, 5, 2);

			assert(subtractionFuture.get(), ==, 8);
			assert(sumFuture.get(), ==, 7);
		};

		test_case("pool should process functions with arguments but no return") {
			thread_pool pool(1);
			int subtractionResult;
			int sumResult;

			auto subtraction = [&](int arg1, int arg2) {
				subtractionResult = arg1 - arg2;
			};

			auto sum = [&](int arg1, int arg2) {
				sumResult = arg1 + arg2;
			};

			auto subtractionFuture = pool.exec(subtraction, 10, 2);
			auto sumFuture = pool.exec(sum, 5, 2);

			subtractionFuture.wait();
			assert(subtractionResult, ==, 8);

			sumFuture.wait();
			assert(sumResult, ==, 7);
		};

		test_case("pool should execute tasks in order of submission") {
			thread_pool pool(1);
			bool executingFirstTask = false;
			bool executingSecondTask = false;

			auto firstTask = [&]() {
				executingFirstTask = true;
				this_thread::sleep_for(15ms);
				assert(executingSecondTask, ==, false);
				executingFirstTask = false;
			};

			auto secondTask = [&]() {
				executingSecondTask = true;
				assert(executingFirstTask, ==, false);
				executingSecondTask = false;
			};

			auto firstTaskFuture = pool.exec(firstTask);
			auto secondTaskFuture = pool.exec(secondTask);

			secondTaskFuture.wait();
			firstTaskFuture.wait();
		};

		test_case("two pools should execute tasks in parallel") {
			thread_pool pool1(1), pool2(1);

			bool executingOneTask = false;

			auto firstTask = [&]() {
				executingOneTask = true;
				this_thread::sleep_for(30ms);
				executingOneTask = false;
			};

			auto secondTask = [&]() {
				this_thread::sleep_for(15ms);
				assert(executingOneTask, ==, true);
			};

			auto firstTaskFuture = pool1.exec(firstTask);
			auto secondTaskFuture = pool2.exec(secondTask);

			firstTaskFuture.wait();
			secondTaskFuture.wait();
		};
	}

	test_suite("when using thread pools of multiple threads") {
		test_case("pool should process functions with arguments and return") {
			thread_pool pool(2);
			auto subtraction = [](int arg1, int arg2) {
				return arg1 - arg2;
			};

			auto sum = [](int arg1, int arg2) -> int {
				return arg1 + arg2;
			};

			auto subtractionFuture = pool.exec(subtraction, 10, 2);
			auto sumFuture = pool.exec(sum, 5, 2);

			assert(subtractionFuture.get(), ==, 8);
			assert(sumFuture.get(), ==, 7);
		};

		test_case("pool should process functions with arguments but no return") {
			thread_pool pool(2);
			int subtractionResult;
			int sumResult;

			auto subtraction = [&](int arg1, int arg2) {
				subtractionResult = arg1 - arg2;
			};

			auto sum = [&](int arg1, int arg2) {
				sumResult = arg1 + arg2;
			};

			auto subtractionFuture = pool.exec(subtraction, 10, 2);
			auto sumFuture = pool.exec(sum, 5, 2);

			subtractionFuture.wait();
			assert(subtractionResult, ==, 8);

			sumFuture.wait();
			assert(sumResult, ==, 7);
		};

		test_case("pool should execute multiple tasks in parallel") {
			thread_pool pool(2);

			bool executingOneTask = false;

			auto firstTask = [&]() {
				executingOneTask = true;
				this_thread::sleep_for(30ms);
				executingOneTask = false;
			};

			auto secondTask = [&]() {
				this_thread::sleep_for(15ms);
				assert(executingOneTask, ==, true);
			};

			auto firstTaskFuture = pool.exec(firstTask);
			auto secondTaskFuture = pool.exec(secondTask);

			firstTaskFuture.wait();
			secondTaskFuture.wait();
		};

		test_case("pool should terminate correctly when no tasks are sent for execution") {
			bool joined = false;
			thread_pool pool(2);

			pool.terminate();
			joined = true;

			assert(joined, ==, true);
		};

		test_case("pool should terminate correctly when number of tasks sent for execution is smaller than number of threads") {
			bool joined = false;
			thread_pool pool(2);

			pool.exec([]{});

			pool.terminate();
			joined = true;

			assert(joined, ==, true);
		};

	}

	test_suite("when manually terminating a thread pool") {
		test_case("pool should drop tasks which have not been consumed from the queue") {
			bool task_dropped = true;
			thread_pool pool(2);

			auto keep_one_thread_busy = [] {
				this_thread::sleep_for(15ms);
			};

			pool.exec(keep_one_thread_busy);
			pool.exec(keep_one_thread_busy);

			pool.exec([&] {
				this_thread::sleep_for(15ms);
				task_dropped = false;
			});
			pool.terminate();

			assert(task_dropped, ==, true);
		};

		test_case("execution should block until tasks which have already been consumed from he queue finish") {
			bool execution_blocked = false;
			thread_pool pool(1);

			pool.exec([&] {
				this_thread::sleep_for(30ms);
				execution_blocked = true;
			});
			this_thread::sleep_for(15ms);
			pool.terminate();

			assert(execution_blocked, ==, true);
		};
	}

	test_suite("when destroying a thread pool") {
		test_case("pool should drop tasks which have not been consumed from the queue") {
			bool task_dropped = true;
			auto keep_one_thread_busy = [] {
				this_thread::sleep_for(15ms);
			};

			{
				thread_pool pool(2);

				pool.exec(keep_one_thread_busy);
				pool.exec(keep_one_thread_busy);

				pool.exec([&] {
					this_thread::sleep_for(15ms);
					task_dropped = false;
				});
			}

			assert(task_dropped, ==, true);
		};

		test_case("execution should block until tasks which have already been consumed from the queue finish") {
			bool execution_blocked = false;
			{
				thread_pool pool(1);

				pool.exec([&] {
					this_thread::sleep_for(30ms);
					execution_blocked = true;
				});
				this_thread::sleep_for(15ms);
			}

			assert(execution_blocked, ==, true);
		};
	}

	test_suite("when stressing a thread pool of 2 threads with 100,000 empty signature tasks") {
		const int tasks_to_execute = 100'000;
		test_case("pool should be able to execute all tasks in less than 150ms") {
			thread_pool pool(2);
			vector<future<void>> futures;
			futures.reserve(tasks_to_execute);

			stopwatch stopwatch;
			for (int i = 0; i < tasks_to_execute; i++) {
				futures.emplace_back(pool.exec([]{}));
			}

			for (auto& future : futures) {
				future.wait();
			}

			assert(stopwatch.lap_time(), <, 300ms);
		};
	}

	test_suite("when stressing a thread pool of 2 threads with 100,000 tasks with args but no return") {
		const int tasks_to_execute = 100'000;
		test_case("pool should be able to execute all tasks in less than 150ms") {
			thread_pool pool(2);
			vector<future<void>> futures;
			futures.reserve(tasks_to_execute);
			vector<int> tasks_results(tasks_to_execute);

			stopwatch stopwatch;
			for (int i = 0; i < tasks_to_execute; i++) {
				futures.emplace_back(
					pool.exec([&, i](int a){
						tasks_results[i] = a;
					}, 1)
				);
			}

			for (auto& future : futures) {
				future.wait();
			}

			assert(stopwatch.lap_time(), <, 300ms);
		};
	}

	test_suite("when stressing a thread pool of 2 threads with 100,000 tasks with args and return") {
		const int tasks_to_execute = 100'000;
		test_case("pool should be able to execute all tasks in less than 150ms") {
			thread_pool pool(2);
			vector<future<int>> futures;
			futures.reserve(tasks_to_execute);
			vector<int> tasks_results(tasks_to_execute);

			stopwatch stopwatch;
			for (int i = 0; i < tasks_to_execute; i++) {
				futures.emplace_back(
					pool.exec([](int a){
						return a;
					}, 1)
				);
			}

			for (auto& future : futures) {
				future.wait();
			}

			assert(stopwatch.lap_time(), <, 300ms);
		};
	}
} end_tests;

