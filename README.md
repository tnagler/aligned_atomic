# `aligned_atomic<T>`

> Cache aligned atomics in C++11


### Problem

Atomic objects are meant to be shared across cores. Read and write (and some read-modify-write) operations are atomic, so there are no data races. However, 
if one core writes it must flush the memory of others. If the other core is 
reading from a second object that shares the same cache line, this causes 
significant overhead (see [false sharing](https://en.wikipedia.org/wiki/False_sharing)).

### Solution

To prevent false sharing, we should 
1. align the object in memory to the beginning of a cache line,
2. add padding bytes so that no other object can occupy this cache line.

C++17 brought [aligned allocation](https://en.cppreference.com/w/cpp/memory/c/aligned_alloc)
facilities previously only available via [boost](https://www.boost.org/doc/libs/1_65_1/doc/html/align.html#align.introduction). Besides being somewhat tedious to use, both
only ensure memory alignment (1), but don't add padding bytes (2). 

**`aligned_atomic<T>`** is a simple specialization of [`std::atomic<T>`](https://en.cppreference.com/w/cpp/atomic/atomic) 
that ensures proper memory alignment and padding (1 + 2). 

### Usage

**`aligned_atomic<T>`** objects behave just like [`std::atomic<T>`](https://en.cppreference.com/w/cpp/atomic/atomic).

``` cpp
#include "aligned_atomic.hpp"

// default alignment is 64 bytes (one cache line on modern computers)
aligned_atomic<size_t> val{0};

// align to 32 bytes
aligned_atomic<size_t, 32> val_32{0};

// some operations
val = 10;
++val;
val.fetch_add(5, std::memory_order_relaxed);

// etc.
```