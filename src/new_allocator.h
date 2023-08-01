//
// Created by xd on 2023/6/13.
// 标准库的写法

#ifndef XD_ALLOCATOR_NEW_ALLOCATOR_H
#define XD_ALLOCATOR_NEW_ALLOCATOR_H

#include <iostream>


namespace xd {

    template<typename T>
    class new_allocator {
    public:
        typedef T value_type;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T &reference;
        typedef const T &const_reference;

        T *allocate(size_type _n, const void * = static_cast<const void *>(0)) {
            return static_cast<T *> (::operator new(_n * sizeof(T)));
        }

        void deallocate(T *_p, size_type _t) {
            ::operator delete(_p);
        }
    };

    template<typename T>
    using allocator = new_allocator<T>;
}
#endif //XD_ALLOCATOR_NEW_ALLOCATOR_H
