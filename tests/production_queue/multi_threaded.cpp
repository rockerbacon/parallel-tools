#include <assertions-test/test.h>
#include <production_queue.h>
#include <thread>

using namespace std;

begin_tests {
	test_suite("when producing and consuming asynchronously") {
		test_case("consumption should block until a resource is available") {
			parallel_tools::production_queue<int> queue(1);
			chrono::high_resolution_clock::duration time_to_consume = 0ms;

			auto begin = chrono::high_resolution_clock::now();
			std::thread consumer_thread([&] {
				queue.consume();
				time_to_consume = chrono::high_resolution_clock::now() - begin;
			});

			std::thread producer_thread([&] {
				this_thread::sleep_for(15ms);
				queue.produce(10);
			});

			consumer_thread.join();
			producer_thread.join();

			assert(time_to_consume, >=, 15ms);
		};

		test_case("production should block until there's space available in the queue") {
			parallel_tools::production_queue<int> queue(1);
			queue.produce(0);
			chrono::high_resolution_clock::duration time_to_produce = 0ms;

			auto begin = chrono::high_resolution_clock::now();
			std::thread producer_thread([&] {
				queue.produce(1);
				time_to_produce = chrono::high_resolution_clock::now() - begin;
			});

			std::thread consumer_thread([&] {
				this_thread::sleep_for(15ms);
				queue.consume();
			});

			producer_thread.join();
			consumer_thread.join();

			assert(time_to_produce, >=, 15ms);
		};

		test_case("consumer should consume in first-in-first-out order") {
			vector<int> resources{ 10, 9, 4, 15 };
			parallel_tools::production_queue<int> queue(1);
			vector<int> consumed_resources;

			std::thread consumer_thread([&] {
				for (size_t i = 0; i < resources.size(); i++) {
					auto consumed_resource = queue.consume();
					assert(consumed_resource, ==, resources[i]);
				}
			});

			std::thread producer_thread([&] {
				for (auto resource : resources) {
					this_thread::sleep_for(2ms);
					queue.produce(resource);
				}
			});

			consumer_thread.join();
			producer_thread.join();
		};
	}

	test_suite("when consuming and producing with multiple producers and consumers") {
		test_case("all resources should be consumed and none should be consumed more than once") {
			int resource_count = 1000;
			vector<atomic<unsigned>> consumption_counts(resource_count);
			int consumers_count = 16;
			vector<thread> consumers;
			int producers_count = 10;
			vector<thread> producers;
			parallel_tools::production_queue<int> queue(consumers_count);
			atomic<int> running_producers(producers_count);

			for (auto& count : consumption_counts) {
				count.store(0);
			}

			for (int i = 0; i < producers_count; i++) {
				producers.emplace_back([&, i] {
					for (int j = i; j < resource_count; j += producers_count) {
						queue.produce(j);
					}
					running_producers--;
					if (running_producers == 0) {
						for (int j = 0; j < consumers_count; j++) {
							queue.produce(-1);
						}
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
		};
	}
} end_tests;

