#pragma once

#include <future>

namespace parallel_tools {

	template<typename T>
	class compound_future {
		private:
			std::shared_ptr<T> return_value;
			std::future<void> future;

		public:
			compound_future() = default;
			compound_future (const decltype(return_value)& return_value, decltype(future)&& future) :
				return_value(return_value),
				future(std::move(future))
			{}

			void wait () {
				future.wait();
			}

			T get() {
				wait();
				return *return_value;
			}
	};
}
