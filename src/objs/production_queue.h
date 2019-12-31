#pragma once

#include <memory>

namespace parallel_tools {
	template<typename resource_type>
	class production_queue {
		private:
			struct node {
				resource_type resource;
				std::shared_ptr<node> next;
			};

			std::shared_ptr<node> head;
			std::shared_ptr<node> tail;
		public:
			void produce(const resource_type& resource) {
				auto previous_tail = std::move(tail);
				tail = std::make_shared<node>(node{
					resource,
					nullptr
				});

				if (head) {
					previous_tail->next = tail;
				} else {
					head = tail;
				}
			}

			resource_type consume () {
				auto resource = std::move(head->resource);
				head = head->next;
				return std::move(resource);
			}
	};
}

