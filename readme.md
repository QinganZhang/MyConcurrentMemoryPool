



每次测试中，随机申请内存10000次，释放内存10000次，申请和释放内存随机交错，重复两轮。大部分申请的内存大小在1KB左右，大块内存申请和释放的次数占比较小（1%，2%，5%）。测试用时单位为毫秒，前面是使用普通的malloc/free进行申请和释放内存，后面使用实现的内存池进行申请和释放内存。

windows平台下：

| 大块内存占比 |     1线程     |     8线程     |    16线程     |     32线程     |     128线程     |
| :----------: | :-----------: | :-----------: | :-----------: | :------------: | :-------------: |
|     0.01     | 0.002 / 0.002 | 0.059 / 0.052 | 0.206 / 0.183 | 0.986 / 0.677  | 44.180 / 10.530 |
|     0.02     | 0.001 / 0.003 | 0.113 / 0.094 | 0.469 / 0.331 | 2.844 / 1.127  |                 |
|     0.05     | 0.003 / 0.005 | 0.335 / 0.186 | 1.916 / 0.779 | 12.974 / 3.151 |                 |

linux平台下：

| 大块内存占比 |        1线程        |     8线程     |    16线程     |     32线程      |     128线程      |
| :----------: | :-----------------: | :-----------: | :-----------: | :-------------: | :--------------: |
|     0.01     | 0.001952 / 0.010024 | 0.275 / 0.605 | 1.462 / 2.126 |  6.498 / 9.038  | 82.624 / 117.272 |
|     0.02     | 0.002512 / 0.013387 | 0.392 / 0.606 | 1.761 / 3.332 |  8.334 / 8.285  |                  |
|     0.05     | 0.004317 / 0.012352 | 0.721 / 1.094 | 3.214 / 4.712 | 18.157 / 19.184 |                  |

可以看出在多线程、申请大量大块内存时，实现的内存池具有较好的性能。