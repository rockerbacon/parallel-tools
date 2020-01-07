## Introduction

Tools for easy multithread programming in C++17.

#### Table of Contents
- [Adding to your Project](#adding-to-your-project)
- [Bulding and Testing](#building-and-testing)
- [Features](#features)
  - [Consumer-Producer Queue](#consumer-producer-queue)
  - [Thread Pool](#thread-pool)
  - [Complex Atomic](#complex-atomic)

## Adding to your Project

In case you're using the [Assertions C++ Framework](https://github.com/rockerbacon/assertions/wiki), simply run the command:

```
./dependencies.sh add git https://github.com/rockerbacon/parallel-tools.git
```

## Building and Testing

This project uses the [Assertions C++ Framework](https://github.com/rockerbacon/assertions/wiki) for all its build, test and external dependencies management.

For running all tests use ```./test.sh```

For building everything use ```./build.sh```. Compiled binaries will be available inside ```./build```

For running the thread pool benchmark use:
```
./run.sh benchmarks/thread_pool/void_signature.cpp
```

## Features

All features are available in the namespace _parallel\_tools_

### Consumer-Producer Queue

The consumer-producer queue is implemented in the template class `production_queue`, defined in the header `production_queue.h`.

The implementation uses a double buffer to reduce locking. One buffer is accessible only to the producers while the other is accessible only to the consumers. The two buffers are eventually swapped according to a dynamic _flush policy_.

A simple usage example:
```C++
parallel_tools::production_queue<int> queue;  // uses default flush policy "always"

queue.produce(5);
auto resource = queue.consume();
std::cout << resource << std::endl; // prints '5'
```

The production is always assured to happen but consumption will block untill a resource is available. Because of that it's important to ensure the chosen flush policy will not cause resources to get stuck in the production buffer and cause consumers to deadlock.

#### Flush Policies

Flush policies determine when the production and consumption buffers will be swapped. The buffers are only swapped when a consumer tries to consume and there are no resources available.

Policies are determined during construction of the queue but can be changed at any time using the method _switch\_policy_:

```C++
parallel_tools::production_queue<int> queue(parallel_tools::flush_policy::never);

queue.switch_policy(parallel_tools::flush_policy::batches_of{4});
```

The following policies are available in the namespace `parallel_tools::flush_policy`:

- `always`: always swap. This is the default policy;
- `never`: never swap. Can be used for more manual control of the buffers;
- `batches_of{n}`: swap when the production buffer has _n_ resources or more;
- `maximum_waiting_consumers{n}`: allow up to _n_ consumers to block, waiting for a resource, before swapping;

Even if the flush policy is set to _always_ the implementation still provides a very substantial performance advantage over using a single buffer. That is validaded by the provided benchmark and can be explained by two main factors:

- Swapping queues is faster than inserting and removing resources in/from the buffer, which causes locks to be held for less time;
- If a single swap causes the consumption buffer to be filled with more resources than there are consumers, consumers and producers will operate without locking each other untill the consumption buffer is emptied again;

### Thread Pool

The thread pool is implemented in the class `thread_pool`, available in the header `thread_pool.h`. It uses the consumer-producer queue to handle tasks in a performant manner. Its usage is extremely simple and versatile:

```C++
unsigned number_of_threads = 4;
parallel_tools::thread_pool pool(number_of_threads);

std::future<int> future = pool.exec([] (int a, int b) {
  return a+b;
}, 2, 4);

auto sum_result = future.get();
std::cout << sum_result << std::endl; // should print '6'
```

The exec method can take functions with virtually any signature:

```C++
std::future<int> future = pool.exec([]{ return 4; });
std::future<void> future2 = pool.exec([]{ /* do nothing */ });
```

Note: the flush policy of the underlying queue can be defined in the pool's constructor as a last optional argument. As of version v1.3, however, there's no interface for changing the policy afterwards which makes the thread pool extremely deadlock prone when using any policy other than _always_.

Note2: allowing the thread pool to be destroyed or manually terminating it with the method `terminate()` before waiting for all futures will cancel execution of any tasks which have not yet been consumed from the queue.

### Complex Atomic

A complex atomic is a simple wrapper which ensures atomic reads and writes. It is implemented in the template class `complex_atomic`, available in the header `complex_atomic.h`.

This is no replacement for `std::atomic` and attempting to use it like so is a pessimization. This is simply a user friendly interface for performing multiple operations atomically while `std::atomic` only ensures atomicity of a single operation.

Complex atomics can have their underlying value modified at any time using the method `access` which takes a function with the signature `void(T&)` as its only argument, where _T_ is the type of the underlying value:

```C++
parallel_tools::complex_atomic<int> atomic_int(0);

atomic_int.access([](int& value) {
  value++;
  value++;
  value++;
  // all three operations will be executed atomicaly
 });
 ```
 
Complex atomics are also convertible to the underlying value. Note, however, that this cast will always cause the underlying value to be copied:

```C++
parallel_tools::complex_atomic<int> atomic_int(3);

int a = atomic_int;

std::cout << a << std::endl; // prints '3'
```
