# testing：测试框架（`mcxx.testing`）

一个最小的具名模块测试框架，是各包 `tests/` 唯一的 dev 依赖。

```cpp
import mcxx.testing;
using namespace mcxx::testing;
int main() {
    "adds"_test = [] { expect(1 + 1 == 2) << "arithmetic"; };
    "stops at a fatal"_test = [] { expect(fatal(true)); };
    return report();
}
```

复制自 mcppls 的 mcppls-testing（同一作者），改名为 mcxx。
