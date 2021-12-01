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

template<class T, size_t Alignment = 64>
struct alignas(Alignment) aligned_atomic : public std::atomic<T>
{
  public:
    aligned_atomic() noexcept = default;

    aligned_atomic(T desired) noexcept
      : std::atomic<T>(desired)
    {}

    // Assignment operators have been deleted, must redefine.
    T operator=(T desired) noexcept
    {
        this->store(desired);
        return desired;
    }

    T operator=(T desired) volatile noexcept
    {
        this->store(desired);
        return desired;
    }
    static void* operator new(size_t count) noexcept
    {
        // Make sure alignment is at least that of void*.
        constexpr size_t alignment =
          (Alignment >= alignof(void*)) ? Alignment : alignof(void*);

        // Compute space required for object, void*, and padding.
        size_t space = count + alignment + sizeof(void*);
        space = (space >= 2 * alignment) ? space : 2 * alignment; // padding

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
        // location. Later we'll know where to look for the pointer telling us
        // where to free what we malloc()'ed above.
        *(static_cast<void**>(p_algn) - 1) = p;

        return p_algn;
    }

    static void operator delete(void* ptr)
    {
        if (ptr) {
            std::free(*(static_cast<void**>(ptr) - 1));
        }
    }
};

