#include <assertions-test/test.h>
#include <production_queue.h>
#include <vector>

using namespace std;
begin_tests {
	test_suite("when producing and consuming a single resource") {
		test_case("consuming should return a resource that was previously produced") {
			int resource = 10;
			parallel_tools::production_queue<decltype(resource)> queue;
			queue.produce(10);
			auto consumed_resource = queue.consume();
			assert(consumed_resource, ==, resource);
		};
	}

	test_suite("when producing and consuming on a new queue") {
		test_case("should work in first-in-first-out order") {
			vector<int> resources{ 10, 9, 15, 4 };
			parallel_tools::production_queue<int> queue;

			for (auto resource : resources) {
				queue.produce(resource);
			}

			for (size_t i = 0; i < resources.size(); i++) {
				auto consumed_resource = queue.consume();
				assert(consumed_resource, ==, resources[i]);
			}
		};
	}

	test_suite("when emptying a queue and then refilling it") {
		test_case("consuming should return items in a first-in-first-out order") {
			// fill queue
			vector<int> resources{ 10, 9, 15, 4 };
			parallel_tools::production_queue<int> queue;
			for (auto resource : resources) {
				queue.produce(resource);
			}

			// empty queue
			for (size_t i = 0; i < resources.size(); i++) {
				queue.consume();
			}

			// refill queue
			for (auto resource : resources) {
				queue.produce(resource);
			}

			for (size_t i = 0; i < resources.size(); i++) {
				auto consumed_resource = queue.consume();
				assert(consumed_resource, ==, resources[i]);
			}
		};
	}

	test_suite("when consuming some items and then producing more") {
		test_case("consuming should return items in a first-in-first-out order") {
			vector<int> resources{ 10, 9, 15, 4 };
			vector<int> resources_after_refill{ 15, 4, 10, 9, 15, 4 };
			parallel_tools::production_queue<int> queue;
			size_t items_to_consume = resources.size()/2;

			for (auto resource : resources) {
				queue.produce(resource);
			}

			for (decltype(items_to_consume) i = 0; i < items_to_consume; i++) {
				queue.consume();
			}

			for (auto resource : resources) {
				queue.produce(resource);
			}

			for (size_t i = 0; i < resources.size(); i++) {
				auto consumed_resource = queue.consume();
				assert(consumed_resource, ==, resources_after_refill[i]);
			}
		};
	}
} end_tests;

