#pragma once

#include <mutex>

namespace parallel_tools {
	template<typename T>
	class complex_atomic {
		private:
			std::mutex mutex;
			T object;

		public:
			template<typename... args_types>
			complex_atomic(args_types&&... args) :
				object(std::forward<args_types>(args)...)
			{}

			operator T () {
				std::lock_guard lock(mutex);
				return T(object);
			}

			template<typename function_type>
			void access (const function_type& function) {
				std::lock_guard lock(mutex);
				function(object);
			}
	};
}
