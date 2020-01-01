#include <stopwatch/stopwatch.h>
#include <cpp-benchmark/benchmark.h>
#include <thread>
#include <vector>

#include <thread_pool.h>

#define MIN_THREADS 4
#define MAX_THREADS 64
#define TASKS_PER_RUN 500'000
#define RUNS 50

#define SETUP_BENCHMARK()\
	TerminalObserver terminal_observer;\
	chrono::high_resolution_clock::duration production_time,\
											consumption_time,\
											run_time;\
	unsigned run;\
	float progress;\
\
	register_observers(terminal_observer);\
\
	observe(progress, percentage_complete);\
\
	observe_average(production_time, average_production_time);\
	observe_minimum(production_time, fastest_production_time);\
	observe_maximum(production_time, slowest_production_time);\
\
	observe_average(consumption_time, average_consumption_time);\
	observe_minimum(consumption_time, fastest_consumpion_time);\
	observe_maximum(consumption_time, slowest_consumption_time);\
\
	observe_average(run_time, average_run_time);\
	observe_minimum(run_time, fastest_run_time);\
	observe_maximum(run_time, slowest_run_time);\


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
			vector<chrono::high_resolution_clock::duration> tasks_consumption_time(TASKS_PER_RUN);
			vector<chrono::high_resolution_clock::duration> tasks_production_time(TASKS_PER_RUN);
			vector<stopwatch> consumption_stopwatches(TASKS_PER_RUN);

			stopwatch run_stopwatch;
			for (unsigned i = 0; i < TASKS_PER_RUN; i++) {
				auto task = [
					&tasks_consumption_time,
					i,
					&consumption_stopwatches
				] () mutable {
					tasks_consumption_time[i] = consumption_stopwatches[i].lap_time();
				};

				stopwatch production_stopwatch;
				consumption_stopwatches[i].reset();
				auto future = pool.exec(task);
				consumption_stopwatches[i].reset();
				tasks_production_time[i] = production_stopwatch.lap_time();

				tasks_futures.emplace_back(std::move(future));
			}

			pool.finish();
			for (auto& future : tasks_futures) {
				future.wait();
			}
			run_time = run_stopwatch.lap_time();
			consumption_time = *max_element(tasks_consumption_time.begin(), tasks_consumption_time.end());
			production_time = *max_element(tasks_production_time.begin(), tasks_production_time.end());

			run++;
			progress = (float)run/RUNS*100.0f;
		}
	}
}
