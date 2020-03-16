#include <stopwatch/stopwatch.h>
#include <cpp-benchmark/benchmark.h>
#include <thread>
#include <vector>

#include <thread_pool.h>

#define MIN_THREADS 4
#define MAX_THREADS 64
#define TASKS_PER_RUN 100'000
#define RUNS 50

#define SETUP_BENCHMARK()\
	TerminalObserver terminal_observer;\
	chrono::high_resolution_clock::duration production_time,\
											consumption_time,\
											delay;\
	unsigned run;\
	float progress;\
\
	register_observers(terminal_observer);\
\
	observe(progress, percentage_complete);\
\
	observe_average(production_time, avg_production_time);\
	observe_minimum(production_time, min_production_time);\
	observe_maximum(production_time, max_production_time);\
\
	observe_average(consumption_time, avg_consumption_time);\
	observe_minimum(consumption_time, min_consumpion_time);\
	observe_maximum(consumption_time, max_consumption_time);\
\
	observe_average(delay, avg_delay);\
	observe_minimum(delay, min_delay);\
	observe_maximum(delay, max_delay);\


using namespace benchmark;
using namespace std;

int main() {
	for (unsigned threads = MIN_THREADS; threads <= MAX_THREADS; threads *= 2) {
		SETUP_BENCHMARK();

		run = 0;
		parallel_tools::thread_pool pool(threads);
		string benchmark_description = "parallel_tools::thread_pool with void() method and "s + to_string(threads) + " threads";
		benchmark(benchmark_description, RUNS) {
			vector<future<void>> tasks_futures; tasks_futures.reserve(TASKS_PER_RUN);

			vector<chrono::high_resolution_clock::duration> consumption_times(TASKS_PER_RUN);

			vector<stopwatch> consumption_stopwatches(TASKS_PER_RUN);
			stopwatch production_stopwatch;
			for (unsigned i = 0; i < TASKS_PER_RUN; i++) {
				auto task = [
					&consumption_time = consumption_times[i],
					&consumption_stopwatch = consumption_stopwatches[i]
				] () {
					consumption_time = consumption_stopwatch.lap_time();
				};

				tasks_futures.emplace_back(pool.exec(task));
			}
			production_time = production_stopwatch.lap_time();

			for (auto& future : tasks_futures) {
				future.wait();
			}

			consumption_time = *max_element(consumption_times.begin(), consumption_times.end());

			delay = consumption_time - production_time;

			run++;
			progress = (float)run/RUNS*100.0f;
		}
	}
}
