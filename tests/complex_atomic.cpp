#include <assertions-test/test.h>
#include <complex_atomic.h>
#include <future>

using namespace std;

begin_tests {
	test_suite("when modifying an atomic object") {
		test_case("object should be correctly modified") {
			parallel_tools::complex_atomic<int> object(0);

			object.access([](auto& object) {
				object = 2;
			});

			assert(object, ==, 2);
		};

		test_case("modification should block if object is already being modified") {
			parallel_tools::complex_atomic<int> object(2);

			bool blocked = true;
			auto future1 = async(launch::async, [&] {
				object.access([](auto& object) {
					this_thread::sleep_for(15ms);
					object = 5;
				});
			});

			auto future2 = async(launch::async, [&] {
				object.access([&](auto& object) {
					object = 7;
					blocked = false;
				});
			});

			this_thread::sleep_for(5ms);
			assert(blocked, ==, true);

			future1.wait();
			future2.wait();
		};
	}

	test_suite("when capturing the value of an atomic object") {
		test_case("should return a copy of the object") {
			parallel_tools::complex_atomic<int> object(2);

			int copy = object;

			object.access([](auto& object) {
				object = 5;
			});

			assert(copy, !=, object);
		};

		test_case("should block if object is being modified") {
			parallel_tools::complex_atomic<int> object(2);

			bool blocked = true;
			auto future1 = async(launch::async, [&] {
				object.access([](auto& object) {
					this_thread::sleep_for(15ms);
					object = 5;
				});
			});

			auto future2 = async(launch::async, [&]() {
				int value = object;
				blocked = false;
				return value;
			});

			this_thread::sleep_for(5ms);
			assert(blocked, ==, true);

			future1.wait();

			assert(future2.get(), ==, 5);
		};
	}
} end_tests;

