#include <assertions-test/test.h>
#include <compound_future.h>

using namespace parallel_tools;
using namespace std;

begin_tests {
	test_suite ("when waiting for compound future") {
		test_case ("code execution should wait until future is fulfilled") {
			shared_ptr<bool> return_value(new bool(false));
			auto task = [&] {
				this_thread::sleep_for(15ms);
				*return_value = true;
			};

			compound_future<bool> future(
				return_value,
				async(launch::async, task)
			);

			future.wait();
			assert(*return_value, ==, true);
		};

		test_case ("compound future should hold value set in return value") {
			shared_ptr<int> return_value(new int(0));
			packaged_task<void()> task([&] {
				*return_value = 10;
			});

			compound_future<int> future(
				return_value,
				task.get_future()
			);

			task();

			assert(future.get(), ==, 10);
		};
	}
} end_tests;
