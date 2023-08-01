//
// Created by xd on 2023/6/13.
// 一个简单粗暴的分配器

#ifndef XD_ALLOCATOR_LOKI_ALLOCATOR_H
#define XD_ALLOCATOR_LOKI_ALLOCATOR_H

#include <iostream>
#include <vector>
#include <cassert>

namespace xd {
    //开启debug模式
//#define xd_debug

    //最底层
    class Chunk {
    public:
        unsigned char blocksAvailable;  //可以使用的block个数
        unsigned char firstAvailableBlock;  //最近的可以使用的block
        unsigned char *pData;  //指向数据块
    public:
        //reset block里面的数据，串起来
        void reset(size_t blockSize, unsigned char blocks) {
#ifdef xd_debug
            assert(blockSize > 0);
            assert(blocks > 0);
            // Overflow check
            assert((blockSize * blocks) / blockSize == blocks);
#endif

            firstAvailableBlock = 0;
            blocksAvailable = blocks;
            unsigned char i = 0;
            unsigned char *p = pData;
            for (; i != blocks; p += blockSize) {
                *p = ++i;
            }
        }

        //初始化一小块block
        //blockSize代表每个block的大小
        //blocks代表block的个数
        void init(size_t blockSize, unsigned char blocks) {
            pData = static_cast<unsigned char *>(malloc(blockSize * blocks));
            reset(blockSize, blocks);
        }

        //释放block数据
        void release() const {
            free(pData);
        }

        //打印内存数组索引
        void printMemory(size_t block_size) {
            for (int i = 0; i < 64; ++i) {
                unsigned char *pResult = pData + (i * block_size);
                printf("%d ", *pResult);
            }
            printf("\n");
        }

        //检查内存是否正确布置
        //只检查没有给出的部分
        void checkMemory(size_t block_size) {
            printMemory(block_size);
            //判断每个可以用的内存索引在64以内
            printf("AvailableBlock:");
            int index=firstAvailableBlock;
            for (int i = 0; i < blocksAvailable; ++i) {
                printf("%d ",index);
                unsigned char *pResult = pData + (index * block_size);
                index=*pResult;
                if(*pResult > 64){
                    int j=*pResult;
                }
                assert(*pResult <= 64);
            }
            printf("\n");
            //判断可以使用内存数量和实际显示相同
        }

        //分配一块空间
        void *allocate(size_t block_size) {
#ifdef xd_debug
            printf("-------------------------------------------------------\n");
            checkMemory(block_size);
            printf("block_size:%zu blocksAvailable: %d firstAvailableBlock:%d\n", block_size, blocksAvailable,
                   firstAvailableBlock);
#endif
            //没可以用的空间
            if (!blocksAvailable) {
                return nullptr;
            }
            unsigned char *pResult = pData + (firstAvailableBlock * block_size);
            firstAvailableBlock = *pResult;//将要释放的内存的内容取出来，作为firstAvailableBlock
            blocksAvailable--;
#ifdef xd_debug
            checkMemory(block_size);
            printf("allocate %x blocksAvailable:%d firstAvailableBlock:%d\n",pResult,blocksAvailable,firstAvailableBlock);
#endif
            return pResult;
        }

        //释放一块空间
        void deallocate(void *p, size_t block_size) {
            unsigned char *toRelease = static_cast<unsigned char *>(p);
#ifdef xd_debug
            assert(p >= pData);
            assert(p < pData + block_size * 64);
            printf("---------------------------------------------\n");
            printf("deallocate %x blocksAvailable:%d firstAvailableBlock:%d\n",p,blocksAvailable,firstAvailableBlock);
            checkMemory(block_size);
            // 检测回收的目标对齐
            assert((toRelease - pData) % block_size == 0);
            //防止两次回收内存，判定要回收的不是可用的内存。
            //回收两次则pdata会错乱，而且blocksAvailable数量会错误。
            if ( 0 < blocksAvailable )
                assert( firstAvailableBlock != (toRelease - pData) / block_size );
#endif

            *toRelease = firstAvailableBlock;
            //这里的括号一定要加，在强制类型转换的时候
            //否则以为精度问题，大于256的就会变成0，从而导致出错，太坑了
            firstAvailableBlock = static_cast<unsigned char>((toRelease - pData) / block_size);
            ++blocksAvailable;
#ifdef xd_debug
            assert(firstAvailableBlock <= 64);
            assert(*toRelease <= 64);
            printf(" blocksAvailable:%d firstAvailableBlock:%d\n",blocksAvailable,firstAvailableBlock);
            checkMemory(block_size);
#endif
        }

    };

    //中间层
    class FixedAllocator {
    private:
        std::vector<Chunk> chunks;  //保存最底层数据的容器
        Chunk *allocChunk = nullptr;  //新分配的Chunk
        Chunk *deallocChunk = nullptr;    //要删除的Chunk
        unsigned char blockSize;    //申请chunks里面的每个元素的大小
        const unsigned char numBlocks = 64;    //Chunk里面元素的个数,目前先设为64
    private:
        //近邻搜索，比较有效率
        //找到指针所在的chunk
        Chunk *vicinityFind(void *p) const {
            if (chunks.empty()) {
                return nullptr;
            }
            //每个chunk数据的大小
            size_t chunkLength = numBlocks * blockSize;
            auto lo = deallocChunk;
            auto hi = deallocChunk + 1;
            auto loBound = &chunks.front();
            auto hiBound = &chunks.back() + 1;
            // Special case: deallocChunk_ is the last in the array
            if (hi == hiBound) {
                hi = nullptr;
            }
            while (true) {
                //兵分两路搜索
                if (lo) {
                    //找到位置
                    if (p >= lo->pData && p < (lo->pData + chunkLength)) {
                        return lo;
                    }
                    if (lo == loBound) {
                        lo = nullptr;
                        if (hi == nullptr) {
                            break;//到这里说明指针不在当前区块，有可能传入的是无效指针
                        }
                    } else {
                        lo--;
                    }
                }

                if (hi) {
                    if (p >= hi->pData && p < (hi->pData + chunkLength)) {
                        return hi;
                    }
                    if (hi == hiBound) {
                        hi = nullptr;
                        if (lo == nullptr) {
                            break;
                        }
                    } else {
                        hi++;
                    }
                }
            }
            return nullptr; //没有找到位置
        }

        //真的回收内存
        //只有两个同时要被回收的时候，才会真的回收，否则先标记待删除
        //每次都回收末尾的资源
        void doDeallocate(void *p) {
            //回收资源
            deallocChunk->deallocate(p, blockSize);
            //说明全部资源都回收了
            //下面有3种情况处理
            if (deallocChunk->blocksAvailable == numBlocks) {
                Chunk *lastChunk = &chunks.back();
#define doDeallocatemyself
#ifndef doDeallocatemyself
                //参考的方法，可能有的内存不会回收
                //情况1,如果最后一个就是当前 chunk
                if (lastChunk == deallocChunk) {
                    //向前看一个位置
                    //检查是否有两个空的chunks
                    if (chunks.size() > 1 && (deallocChunk - 1)->blocksAvailable == numBlocks) {
                        lastChunk->release();
                        chunks.pop_back();
                        allocChunk = deallocChunk = &chunks.front();
                    }
                    return;
                }
                //到这里最少有两个可以用的,并且最后一个不是当前的 chunks.size()肯定>=2
                if (lastChunk->blocksAvailable == numBlocks) {
                    //情况2，两个空的chunks，最后一个是空的，抛弃最后一个
                    lastChunk->release();
                    chunks.pop_back();
                    allocChunk = deallocChunk;
                    // 这里是不是少一个交换的方法。我觉得应该是
                } else {
                    //情况3,一个空的chunks，将free chunk和最后一个对换位置
                    std::swap(*deallocChunk, *lastChunk);
                    allocChunk = &chunks.back();
                }
#endif
#ifdef doDeallocatemyself
                //自己写的方法 始终保持当前最多有一个空的地方
                //情况1,如果最后一个就是当前 deallocChunk,直接返回
                if (lastChunk == deallocChunk) {
                    allocChunk = deallocChunk;
                    return;
                }
                //到这里最少有两个可以用的（chunks.size()肯定>=2）,并且最后一个不是当前的
                if (lastChunk->blocksAvailable == numBlocks) {
                    //情况2，两个空的chunks，最后一个是空的，抛弃最后一个
                    lastChunk->release();
                    chunks.pop_back();
                    //如果当前不是最后，就调整到最后
                    lastChunk = &chunks.back(); //重新定位最后一个。
                    if (deallocChunk != lastChunk) {
                        std::swap(*deallocChunk, *lastChunk);
                    }
                    allocChunk = deallocChunk;
                } else {
                    //情况3,一个空的chunks，将free chunk和最后一个对换位置
                    std::swap(*deallocChunk, *lastChunk);
                    allocChunk = deallocChunk;
                }
#endif
            }
        }

    public:
        explicit FixedAllocator(unsigned char blockSize) : blockSize(blockSize) {}


        void *allocate() {
            if (allocChunk == nullptr || allocChunk->blocksAvailable == 0) {
                //目前allocChunk没有可以用的空间
                auto i = chunks.begin();
                for (;; ++i) {
                    //直到到尾部没有找到
                    if (i == chunks.end()) {
                        //初始化一个新的空间
                        chunks.emplace_back();
                        Chunk &newChunk = chunks.back();
                        newChunk.init(blockSize, numBlocks);
                        allocChunk = &newChunk;
                        //有可能重新转移容器里面元素，导致之前指针失效，所以直接赋值
                        deallocChunk = &chunks.front();
                        break;
                    }
                    if (i->blocksAvailable > 0) {
                        //找到可以用的空间
                        allocChunk = &(*i);//取地址
                        break;
                    }
                }
            }
            return allocChunk->allocate(blockSize);
        }

        void deallocate(void *p) {
            //每次删除前会从新找到合适的位置
            deallocChunk = vicinityFind(p);
            doDeallocate(p);
        }

    };

    //上层，使用单例模式
    //小于等于128bytes的用当前分配器管理，大于这个的直接用malloc管理
    //共有8-128，16个FixedAllocator，分别管理8 16 ... 128字节的分配。
    class SmallObjectAllocator {
    private:
        const size_t MAX_BYTES = 128;   //小区块的上限
        const size_t ALIGN = 8; //小区块的上调边界
        const size_t FixedAllocatorNUM = MAX_BYTES / ALIGN;    //FixedAllocator个数
        size_t chunkSize;   //每个内存块大小
        std::vector<FixedAllocator> pool;   //包含中间层的容器
        FixedAllocator *pLastAlloc; //指向最近访问的，创建的FixedAllocator;
        FixedAllocator *pLastDealloc;   //指向最近的清理过的FixedAllocator;
    private:
        //将某个数上调为8的倍数，如13->16  20->24
        size_t ROUND_UP(size_t bytes) const {
            return ((bytes) + ALIGN - 1) & ~(ALIGN - 1);
        }

        // 计算某个数所在的位置索引 例如：5：0  10：1  24：2
        size_t FIND_INDEX(size_t bytes) const {
            return (bytes + ALIGN - 1) / ALIGN - 1;
        }

        SmallObjectAllocator() {
            pool.reserve(FixedAllocatorNUM);
            //初始化pool
            for (size_t i = ALIGN; i <= MAX_BYTES; i += ALIGN) {
                pool.emplace_back(i);
            }
        }

    public:
        static SmallObjectAllocator &getInstance() {
            static SmallObjectAllocator smallObjectAllocator;
            return smallObjectAllocator;
        }

        // 禁止复制和赋值
        SmallObjectAllocator(const SmallObjectAllocator &) = delete;

        SmallObjectAllocator &operator=(const SmallObjectAllocator &) = delete;

        //分配内存
        void *allocate(size_t n) {
            //这个分配器只管理 小于等于128字节的区块
            if (n > MAX_BYTES) {
                return malloc(n);
            }
            return pool[FIND_INDEX(n)].allocate();
        }

        //释放内存
        void deallocate(void *p, size_t n) {
            //这个分配器只管理 小于等于128字节的区块
            if (n > MAX_BYTES) {
                free(p);
                return;
            }
            pool[FIND_INDEX(n)].deallocate(p);
        }

        //重新分配内存
        void *reallocate(void *p, size_t old_sz, size_t new_sz) {
            pool[FIND_INDEX(old_sz)].deallocate(p);
            return pool[FIND_INDEX(new_sz)].allocate();
        }
    };

    //封装层
    template<typename T>
    class Loki_allocator {
    public:
        typedef T value_type;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T &reference;
        typedef const T &const_reference;
    public:
        T *allocate(size_type _n, const void * = static_cast<const void *>(0)) {
            return static_cast<T *> (SmallObjectAllocator::getInstance().allocate(_n * sizeof(T)));
        }

        void deallocate(T *_p, size_type _n) {
            SmallObjectAllocator::getInstance().deallocate(_p, sizeof(T) * _n);
        }
    };
}

#endif //XD_ALLOCATOR_LOKI_ALLOCATOR_H
