#pragma once

#include <stdlib.h>
#include <list>
#include <thread>
#include <vector>
#include "reusable_thread.h"

namespace parallel {
	unsigned usable_threads (unsigned number_of_items, unsigned number_of_threads) {
		if (number_of_items < number_of_threads) {
			return 1;
		} else {
			return number_of_threads;
		}
	}
}

#define using_threads(pile)\
	auto [parallel_threads_begin, parallel_threads_end] = (::parallel::thread_pile::slice_t)pile;

#define parallel_for(begin, end) {\
\
	auto parallel_number_of_threads = parallel_threads_end - parallel_threads_begin; \
	::std::vector<::std::future<void>> parallel_for_futures(parallel_number_of_threads); \
\
	decltype(parallel_number_of_threads) parallel_for_items_per_thread = (end-begin)/parallel_number_of_threads; \
\
	for (auto [thread, thread_i_name] = ::std::make_tuple(parallel_threads_begin, 0u); thread < parallel_threads_end; thread++, thread_i_name++) { \
\
		auto thread_i = thread_i_name; \
		auto thread_begin = begin + parallel_for_items_per_thread*thread_i; \
		decltype(thread_begin) thread_end; \
		if (thread_i == parallel_number_of_threads-1) { \
			thread_end = end; \
		} else { \
			thread_end = thread_begin + parallel_for_items_per_thread; \
		} \
\
		parallel_for_futures[thread_i] = thread->exec([&, thread_begin, thread_end, thread_i_capture = thread_i](void) { \
			[[maybe_unused]] auto thread_i = thread_i_capture; \
			for (auto i = thread_begin; i < thread_end; i++) \

#define end_parallel_for \
		}); \
	} \
\
	for (auto &future : parallel_for_futures) { \
		future.wait(); \
	} \
}

#define for_each_thread {\
\
	auto parallel_number_of_threads = parallel_threads_end - parallel_threads_begin; \
	::std::vector<::std::future<void>> parallel_for_futures(parallel_number_of_threads); \
\
	for (auto [thread, thread_i_name] = ::std::make_tuple(parallel_threads_begin, 0u); thread < parallel_threads_end; thread++, thread_i_name++) { \
		unsigned thread_i = thread_i_name; \
		parallel_for_futures[thread_i] = thread->exec([&, thread_i](void) \

#define end_for_each_thread \
	);} \
\
	for (auto &future : parallel_for_futures) { \
		future.wait(); \
	} \
\
}

