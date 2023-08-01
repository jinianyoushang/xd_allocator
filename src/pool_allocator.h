//
// Created by xd on 2023/6/13.
// 基于GUN4.9默认__gnu_cxx::__pool_alloc写的pool_allocator
// 其在GUN2.9是默认的分配器
// 可以节省很多cookie的空间，使用了嵌入式指针
// gun2.9有两级的分配器，在后面的地方都删除了第第一级，这里使用第二级。
// 分配释放基本都是O（1）的

#ifndef XD_ALLOCATOR_POOL_ALLOCATOR_H
#define XD_ALLOCATOR_POOL_ALLOCATOR_H

#include <cstdlib>  //for malloc(),realloc()
#include <cstddef>  //for size_t
#include <memory.h>  //for memcpy()

namespace xd {
    //第二级分配器
    static const size_t ALIGN = 8; //小区块的上调边界
    static const size_t MAX_BYTES = 128;   //小区块的上限
    static const size_t NFREELISTS = MAX_BYTES / ALIGN;    //freelist个数
    //分配器 里面单位都是字节  void *
    class default_pool_allocator {
    private:
        //将某个数上调为8的倍数，如13->16  20->24
        static size_t ROUND_UP(size_t bytes) {
            return ((bytes) + ALIGN -1) & ~(ALIGN - 1);
        }

    private:
        struct obj {
            struct obj *next;
        };
    private:
        static struct obj *volatile free_list[NFREELISTS];

        // 计算某个数所在的freelist位置 例如：5：0  10：1  24：2
        static size_t FREELIST_INDEX(size_t bytes) {
            return (bytes + ALIGN - 1) / ALIGN - 1;
        }

        //充值pool的大小 n已经是8的倍数
        static void *refill(size_t n);

        //chunk表示一大块
        //分配一大块内存 nobjs是引用传递
        static char *chunk_alloc(size_t size, size_t &nobjs);

        //pool代表可以用的内存块。
        static char *start_free;    //pool的头部
        static char *end_free;  //pool的尾部
        static size_t heap_size;    //累计分配量
    public:
        static void *allocate(size_t n) {
            obj *volatile *my_free_list;//obj**
            obj *result;
            //这个分配器只管理 小于等于128字节的区块
            if (n > MAX_BYTES) {
                return malloc(n);
            }
            my_free_list = free_list + FREELIST_INDEX(n);
            result = *my_free_list;
            if (result == nullptr) {
                void *r = refill(ROUND_UP(n));
                return r;
            }
            //到这里说明有可以用的内存了,my_free_list指向下一个，并且返回前面所指的内存既可以。
            *my_free_list = result->next;
            return result;
        }

        //相当于把回收的内存串到应的链表头。
        static void deallocate(void *p, size_t n) {
            obj *q = (obj *) p;
            obj *volatile *myfree_list;//obj**
            if (n > MAX_BYTES) {
                free(p);
                return;
            }
            myfree_list = free_list + FREELIST_INDEX(n);
            q->next = *myfree_list;
            *myfree_list = q;
        }

        //重新分配内存
        static void *reallocate(void *p, size_t old_sz, size_t new_sz) {
            void *result;
            size_t copy_sz;
            if (old_sz > MAX_BYTES && new_sz > MAX_BYTES) {
                return realloc(p, new_sz);
            }
            if (ROUND_UP(old_sz) == ROUND_UP(new_sz)) return p;
            result = allocate(new_sz);
            copy_sz = new_sz > old_sz ? old_sz : new_sz;
            memcpy(result, p, copy_sz);
            deallocate(p, old_sz);
            return result;
        }
    };

    char *default_pool_allocator::start_free = nullptr;
    char *default_pool_allocator::end_free = nullptr;
    size_t default_pool_allocator::heap_size = 0;
    default_pool_allocator::obj *volatile default_pool_allocator::free_list[NFREELISTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                                                           0, 0, 0, 0, 0, 0,};

    void *default_pool_allocator::refill(size_t n) {
        size_t NJOBS = 20;   //期望新申请块内存的个数
        char *chunk = chunk_alloc(n, NJOBS);
        obj *volatile *my_free_list;
        obj *result;
        obj *current_obj;
        obj *next_obj;
        if (1 == NJOBS)return chunk;//如果得到的是1，直接返回给客户
        //将得到的链接到freelist上
        my_free_list = free_list + FREELIST_INDEX(n);
        //在chunk中建立 freelist
        result = (obj *) chunk;
        *my_free_list = next_obj = (obj *) (chunk + n);
        for (int i = 1;; ++i) {
            current_obj = next_obj;
            next_obj = (obj *) ((char *) next_obj + n);//找到下一个obj的位置
            if (NJOBS - 1 == i) {//最后一个位置
                current_obj->next = nullptr;
                break;
            } else {
                current_obj->next = next_obj;
            }
        }
        return result;
    }

    char *default_pool_allocator::chunk_alloc(size_t size, size_t &nobjs) {
        char *result;
        size_t total_bytes = size * nobjs;
        size_t bytes_left = end_free - start_free; //直接计算剩余的空间

        if (bytes_left >= total_bytes) {//如果pool剩余的空间大于20倍需要的空间
            result = start_free;
            start_free += total_bytes;
            return result;
        } else if (bytes_left >= size) {//如果pool剩余的空间在1-20倍需要的空间
            nobjs = bytes_left / size;
            total_bytes = size * nobjs;
            result = start_free;
            start_free += total_bytes;
            return result;
        } else {//如果剩余的空间不够1倍 需要的空间
            //新申请大小，这个会随着分配的数量越来越大，也就是每次分配越来越多
            size_t bytes_to_get =
                    2 * total_bytes + ROUND_UP(heap_size >> 4);
            //如果有内存碎片,回收
            if (bytes_left > 0) {
                //找到对应位置。
                //单向链表，头插法
                obj *volatile *my_free_list = free_list + FREELIST_INDEX(bytes_left);
                ((obj *) start_free)->next = *my_free_list;
            }
            //TODO 修改为 ::operate new()
            start_free = (char *) malloc(bytes_to_get);
            //到这里说明新申请失败，则利用已经申请的内存进行分配
            //大部分情况都不会到这里。
            if (nullptr == start_free) {
                obj *volatile *my_free_list;
                obj *p;
                //这里的i代表一个区块大小
                for (size_t i = size; i < MAX_BYTES; i += ALIGN) {
                    my_free_list = free_list + FREELIST_INDEX(i);
                    p = *my_free_list;
                    if (nullptr != p) {
                        *my_free_list = p->next;
                        start_free = (char *) p;
                        end_free = start_free + i;
                        return chunk_alloc(size, nobjs);
                    }
                }
                //到这里说明可以抛出异常了
//                end_free = nullptr;
//                start_free = (char *) malloc(bytes_to_get);
                exit(-1);
            }

            heap_size += bytes_to_get;
            end_free = start_free + bytes_to_get;
            return chunk_alloc(size, nobjs);
        }
    }

    template<typename T>
    class pool_allocator {
    private:

    public:
        typedef T value_type;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T &reference;
        typedef const T &const_reference;

        //这里分配的是字节
        T *allocate(size_type _n, const void * = static_cast<const void *>(0)) {
            return static_cast<T *> (default_pool_allocator::allocate(_n * sizeof(T)));
        }

        void deallocate(T *_p, size_type _n) {
            default_pool_allocator::deallocate(_p, sizeof(T) * _n);
        }
    };

}
#endif //XD_ALLOCATOR_POOL_ALLOCATOR_H
