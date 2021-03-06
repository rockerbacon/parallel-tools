#include <assertions-test/test.h>
#include <production_queue.h>
#include <future>
#include <stopwatch/stopwatch.h>

using namespace std;

begin_tests {
	test_suite("when producing and consuming asynchronously") {
		test_case("consumption should block until a resource is available") {
			parallel_tools::production_queue<int> queue;
			chrono::high_resolution_clock::duration time_to_consume = 0ms;

			auto begin = chrono::high_resolution_clock::now();
			auto consumer_future = async(launch::async, [&] {
				queue.consume();
				time_to_consume = chrono::high_resolution_clock::now() - begin;
			});

			auto producer_future = async(launch::async, [&] {
				this_thread::sleep_for(15ms);
				queue.produce(10);
			});

			consumer_future.wait();
			producer_future.wait();

			assert(time_to_consume, >=, 15ms);
		};

		test_case("production should never block") {
			parallel_tools::production_queue<int> queue;
			bool blocked = false;

			auto future = async(launch::async, [&] {
				queue.produce(0);
				this_thread::sleep_for(15ms);
				blocked = true;
			});

			this_thread::sleep_for(15ms);
			queue.produce(1);

			assert(blocked, ==, false);

			future.wait();

		};

		test_case("consumer should consume in first-in-first-out order") {
			vector<int> resources{ 10, 9, 4, 15 };
			parallel_tools::production_queue<int> queue;
			vector<int> consumed_resources;

			auto consumer_future = async(launch::async, [&] {
				for (size_t i = 0; i < resources.size(); i++) {
					auto consumed_resource = queue.consume();
					assert(consumed_resource, ==, resources[i]);
				}
			});

			auto producer_future = async(launch::async, [&] {
				for (auto resource : resources) {
					this_thread::sleep_for(2ms);
					queue.produce(resource);
				}
			});

			consumer_future.wait();
			producer_future.wait();
		};
	}

	test_suite("when working with a batches") {
		test_case("consumption should block until a batch of the specified size is produced") {
			parallel_tools::production_queue<int> queue(parallel_tools::flush_policy::batches_of{2});
			bool blocked = true;

			auto future = async(launch::async, [&] {
				queue.consume();
				blocked = false;
			});
			this_thread::sleep_for(5ms);

			queue.produce(10);
			this_thread::sleep_for(5ms);
			assert(blocked, ==, true);

			queue.produce(5);
			this_thread::sleep_for(5ms);
			assert(blocked, ==, false);

			future.wait();
		};
	}

	test_suite("when working with a maximum number of waiting consumers") {
		test_case("consumption should block until waiting consumers exceed the maximum") {
			parallel_tools::production_queue<int> queue(parallel_tools::flush_policy::maximum_waiting_consumers{1});
			bool blocked = true;

			auto future1 = async(launch::async, [&] {
				queue.consume();
				blocked = false;
			});
			this_thread::sleep_for(5ms);

			queue.produce(10);
			queue.produce(15);
			this_thread::sleep_for(5ms);
			assert(blocked, ==, true);

			auto future2 = async(launch::async, [&] {
				queue.consume();
				blocked = false;
			});
			this_thread::sleep_for(5ms);
			assert(blocked, ==, false);

			future1.wait();
			future2.wait();
		};
	}

	test_suite("when switching policies") {
		test_case("should unblock any blocked consumers if new maximum waiting consumers policy allows it") {
			parallel_tools::production_queue<int> queue(parallel_tools::flush_policy::never);

			queue.produce(10);
			queue.produce(5);

			bool consumer1_blocked = true;
			auto future1 = async(launch::async, [&] {
				queue.consume();
				consumer1_blocked = false;
			});

			bool consumer2_blocked = true;
			auto future2 = async(launch::async, [&] {
				queue.consume();
				consumer2_blocked = false;
			});

			this_thread::sleep_for(5ms);
			assert(consumer1_blocked, ==, true);
			assert(consumer2_blocked, ==, true);

			queue.switch_policy(parallel_tools::flush_policy::maximum_waiting_consumers{1});
			this_thread::sleep_for(5ms);
			assert(consumer1_blocked, ==, false);
			assert(consumer2_blocked, ==, false);

			future1.wait();
			future2.wait();
		};

		test_case("should unblock any blocked consumers if new batch policy allows it") {
			parallel_tools::production_queue<int> queue(parallel_tools::flush_policy::never);

			queue.produce(10);
			queue.produce(5);

			bool consumer1_blocked = true;
			auto future1 = async(launch::async, [&] {
				queue.consume();
				consumer1_blocked = false;
			});

			bool consumer2_blocked = true;
			auto future2 = async(launch::async, [&] {
				queue.consume();
				consumer2_blocked = false;
			});

			this_thread::sleep_for(5ms);
			assert(consumer1_blocked, ==, true);
			assert(consumer2_blocked, ==, true);

			queue.switch_policy(parallel_tools::flush_policy::batches_of{2});
			this_thread::sleep_for(5ms);
			assert(consumer1_blocked, ==, false);
			assert(consumer2_blocked, ==, false);

			future1.wait();
			future2.wait();
		};
	}

	test_suite("when stressing production queue with 2 consumers, 2 producers and 1,000,000 resources") {
		const int resources_count = 1'000'000;
		const int consumers_count = 2;
		const int producers_count = 2;

		test_case("all resources should be consumed only once in less than 500ms") {
			vector<atomic<unsigned>> consumption_counts(resources_count);
			vector<thread> consumers;
			vector<thread> producers;
			parallel_tools::production_queue<int> queue(parallel_tools::flush_policy::maximum_waiting_consumers{consumers_count-1});
			atomic<int> running_producers(producers_count);

			for (auto& count : consumption_counts) {
				count.store(0);
			}

			stopwatch stopwatch;
			for (int i = 0; i < producers_count; i++) {
				producers.emplace_back([&, i] {
					for (int j = i; j < resources_count; j += producers_count) {
						queue.produce(j);
					}
					running_producers--;
					if (running_producers == 0) {
						for (int j = 0; j < consumers_count; j++) {
							queue.produce(-1);
						}
						queue.switch_policy(parallel_tools::flush_policy::always);
					}
				});
			}

			for (int i = 0; i < consumers_count; i++) {
				consumers.emplace_back([&] {
					while (true) {
						auto count_index = queue.consume();
						if (count_index == -1) break;
						consumption_counts[count_index]++;
					}
				});
			}

			for (auto& consumer : consumers) {
				consumer.join();
			}
			for (auto& producer : producers) {
				producer.join();
			}

			for (auto& consumption_count : consumption_counts) {
				assert(consumption_count, ==, 1);
			}
			assert(stopwatch.lap_time(), <=, 500ms);
		};

		test_case("production should take no more than 250ms") {
			vector<thread> producers;
			parallel_tools::production_queue<int> queue;

			stopwatch stopwatch;
			for (int i = 0; i < producers_count; i++) {
				producers.emplace_back([&, i] {
					for (int j = i; j < resources_count; j += producers_count) {
						queue.produce(j);
					}
				});
			}

			for (auto& producer : producers) {
				producer.join();
			}

			assert(stopwatch.lap_time(), <=, 250ms);
		};

		test_case("consumption should take no more than 250ms") {
			vector<thread> consumers;
			vector<thread> producers;
			parallel_tools::production_queue<int> queue;

			for (int i = 0; i < producers_count; i++) {
				producers.emplace_back([&, i] {
					for (int j = i; j < resources_count; j += producers_count) {
						queue.produce(j);
					}
				});
			}
			for (auto& producer : producers) {
				producer.join();
			}

			stopwatch stopwatch;
			for (int i = 0; i < consumers_count; i++) {
				for (int i = 0; i < resources_count/consumers_count; i++) {
					queue.consume();
				}
			}

			for (auto& consumer : consumers) {
				consumer.join();
			}

			assert(stopwatch.lap_time(), <=, 250ms);
		};
	}
} end_tests;

