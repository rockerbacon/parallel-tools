#include <assertions-test/test.h>
#include <reusable_thread.h>

using namespace parallel_tools;
using namespace std;

begin_tests {
	test_suite("when sending tasks to reusable threads") {
		test_case("threads should be able to execute more than one task") {
			reusable_thread thread;
			int processed_value = 0;

			auto future = thread.exec([&]() {
				processed_value = 10;
			});

			future.wait();

			assert(processed_value, ==, 10);

			future = thread.exec([&]() {
				processed_value = 15;
			});

			future.wait();

			assert(processed_value, ==, 15);
		};

		test_case("threads should process functions with arguments and return") {
			reusable_thread thread;
			auto subtraction = [](int arg1, int arg2) {
				return arg1 - arg2;
			};

			auto sum = [](int arg1, int arg2) -> int {
				return arg1 + arg2;
			};

			auto subtractionFuture = thread.exec(subtraction, 10, 2);
			auto sumFuture = thread.exec(sum, 5, 2);

			assert(subtractionFuture.get(), ==, 8);
			assert(sumFuture.get(), ==, 7);
		};

		test_case("threads should process functions with arguments but no return") {
			reusable_thread thread;
			int subtractionResult;
			int sumResult;

			auto subtraction = [&](int arg1, int arg2) {
				subtractionResult = arg1 - arg2;
			};

			auto sum = [&](int arg1, int arg2) {
				sumResult = arg1 + arg2;
			};

			auto subtractionFuture = thread.exec(subtraction, 10, 2);
			auto sumFuture = thread.exec(sum, 5, 2);

			subtractionFuture.wait();
			assert(subtractionResult, ==, 8);

			sumFuture.wait();
			assert(sumResult, ==, 7);
		};

		test_case("threads should execute tasks in order of submission") {
			reusable_thread thread;
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

			auto firstTaskFuture = thread.exec(firstTask);
			auto secondTaskFuture = thread.exec(secondTask);

			secondTaskFuture.wait();
			firstTaskFuture.wait();
		};

		test_case("two threads should execute tasks in parallel") {
			reusable_thread thread1, thread2;

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

			auto thread1Future = thread1.exec(firstTask);
			auto thread2Future = thread2.exec(secondTask);

			thread1Future.wait();
			thread2Future.wait();
		};

		test_case("a thread's future should still be valid after the reusable_thread object is destroyed") {
			compound_future<int> future;
			{
				reusable_thread thread;
				future = thread.exec([]() { return 10; });
			}

			assert(future.get(), ==, 10);
		};
	}
} end_tests;

