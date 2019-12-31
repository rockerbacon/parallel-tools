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

			std::shared_ptr<node> ground;
			std::reference_wrapper<std::shared_ptr<node>> head;
			std::reference_wrapper<std::shared_ptr<node>> tail;
		public:
			production_queue() :
				ground(new node {
					resource_type(),
					nullptr
				}),
				head(ground->next),
				tail(ground->next)
			{}

			void produce(const resource_type& resource) {
				std::shared_ptr<node>& new_node = tail.get();
				new_node = std::make_shared<node>(node{
					resource,
					nullptr
				});
				tail = new_node->next;
			}

			resource_type consume () {
				auto resource = std::move(head.get()->resource);
				head = head.get()->next;
				return std::move(resource);
			}
	};
}

