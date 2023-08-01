# xd_allocator
A allocator based Loki allocator and pool allocator.

## 说明

本项目实现了gcc2.9中pool allocator，Loki库中的分配器，以及new_allocator。

并且基于这些分配器，实现了自己的分配器（xd::Loki_allocator）见Loki_allocator.h，并对比测试了常见的分配器。

## 测试分配器

自己设置一个benchmark，使用googlebenchmark测试。

8 16 32 64 128 256 

测试1：各自申请8-256字节大小内存，之后立即回收，重复50w次。

测试2：依次申请(8-256)，回收后申请下次,50w次。

测试3：全部申请8-256*50w之后，回收。

测试4：随机全部申请8-256大小的内存，200w次，回收

测试时间和占用的空间

#### 测试空间

linux下用getrusage，windows下用
GetProcessMemorylnfo，算出前后内存占用的差值?

#### 结果-debug模式-50w

|            模式             |  时间s   | 空间(test4)MB |
| :-------------------------: | :------: | :-----------: |
|       std::allocator        |  0.5699  |    194.684    |
|      xd::new_allocator      | 0.561969 |    194.027    |
|     xd::pool_allocator      | 0.325689 |    91.6602    |
|     xd::Loki_allocator      | 1.35296  |    173.531    |
| __gnu_cxx::malloc_allocator | 0.567796 |    194.734    |
|   __gnu_cxx::__pool_alloc   | 0.558766 |    94.6875    |
|    __gnu_cxx::__mt_alloc    | 0.582205 |    39.3008    |
| __gnu_cxx::bitmap_allocator | 0.555472 |    145.332    |

#### 结果-release模式-50w

|            模式             |  时间s   | 空间(test4)MB(仅参考) |
| :-------------------------: | :------: | :-------------------: |
|       std::allocator        | 0.464917 |        194.434        |
|      xd::new_allocator      | 0.456581 |        193.68         |
|     xd::pool_allocator      | 0.156068 |        91.7031        |
|     xd::Loki_allocator      | 0.29328  |        174.961        |
| __gnu_cxx::malloc_allocator | 0.461633 |        194.496        |
|   __gnu_cxx::__pool_alloc   | 0.361838 |        94.6445        |
|    __gnu_cxx::__mt_alloc    | 0.368162 |        39.2852        |
| __gnu_cxx::bitmap_allocator | 0.419301 |        145.391        |

#### 总结

pool_allocator又快又好（占用空间小，而且cookie很少），但是不能回收内存。

new_allocator和std::allocator几乎一样。

xd::Loki_allocator在debug模式较慢，但是在release中很快，仅次于xd::pool_allocator，而且可以自动回收内存。而且运行过程占用空间小，解决了pool_allocator不能回收内存的痛点。