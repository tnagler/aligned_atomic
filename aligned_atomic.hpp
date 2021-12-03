// Copyright (c) 2021 Thomas Nagler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <atomic> // std::atomic
#include <memory> // std::align

// Padding char[]s always must hold at least one char. If the size of the object
// ends at an alignment point, we don't want to pad one extra byte however.
// The construct below ensures that padding bytes are only added if necessary.
namespace padding_impl {

// constexpr modulo operator.
constexpr size_t
mod(size_t a, size_t b)
{
    return a - b * (a / b);
}

// Padding bytes from end of aligned object until next alignment point. char[]
// must hold at least one byte.
template<class T, size_t Align>
struct padding_bytes
{
    static constexpr size_t free_space =
      Align - mod(sizeof(std::atomic<T>), Align);
    static constexpr size_t required = free_space > 1 ? free_space : 1;
    char padding_[required];
};

struct empty_struct
{};

// Class holding padding bytes is necessary. Classes can inherit from this
// to automically add padding if necessary.
template<class T, size_t Align>
struct padding
  : std::conditional<mod(sizeof(std::atomic<T>), Align) != 0,
                     padding_bytes<T, Align>,
                     empty_struct>::type
{};

} // end namespace padding_impl

// Memory-aligned atomic `std::atomic<T>`. Behaves like `std::atomic<T>`, but
// overloads operators `new` and `delete` to align its memory location. Padding
// bytes are added if necessary.
template<class T, size_t Align = 64>
struct alignas(Align) aligned_atomic
  : public std::atomic<T>
  , private padding_impl::padding<T, Align>
{
  public:
    aligned_atomic() noexcept = default;

    aligned_atomic(T desired) noexcept
      : std::atomic<T>(desired)
    {}

    // Assignment operators have been deleted, must redefine.
    T operator=(T x) noexcept { return std::atomic<T>::operator=(x); }
    T operator=(T x) volatile noexcept { return std::atomic<T>::operator=(x); }

    // The alloc/dealloc mechanism is pretty much
    // https://www.boost.org/doc/libs/1_76_0/boost/align/detail/aligned_alloc.hpp
    static void* operator new(size_t count) noexcept
    {
        // Make sure alignment is at least that of void*.
        constexpr size_t alignment =
          (Align >= alignof(void*)) ? Align : alignof(void*);

        // Allocate enough space required for object and a void*.
        size_t space = count + alignment + sizeof(void*);
        void* p = std::malloc(space);
        if (p == nullptr) {
            return nullptr;
        }

        // Shift pointer to leave space for void*.
        void* p_algn = static_cast<char*>(p) + sizeof(void*);
        space -= sizeof(void*);

        // Shift pointer further to ensure proper alignment.
        (void)std::align(alignment, count, p_algn, space);

        // Store unaligned pointer with offset sizeof(void*) before aligned
        // location. Later we'll know where to look for the pointer telling
        // us where to free what we malloc()'ed above.
        *(static_cast<void**>(p_algn) - 1) = p;

        return p_algn;
    }

    static void operator delete(void* ptr)
    {
        if (ptr) {
            // Read pointer to start of malloc()'ed block and free there.
            std::free(*(static_cast<void**>(ptr) - 1));
        }
    }
};
