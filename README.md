# 火车票管理系统

SJTU CS1951 课程大作业

## 项目简介

本项目实现了一个仿 [12306](https://www.12306.cn/) 的火车票订票系统后端，支持用户注册登录、车次管理、车票查询与购买、换乘查询、订单管理及候补购票等功能。

系统通过标准输入输出进行交互，所有数据持久化存储在本地文件中，内存使用受严格限制，支持程序多次重启后数据不丢失。

## 构建与运行

### 环境要求

- C++20 编译器（GCC ≥ 10 或 Clang ≥ 12）
- CMake ≥ 3.10

### 编译

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

编译产物为 `build/code`（可执行文件）。

### 运行

```bash
./build/code < input.txt > output.txt
```

程序从标准输入读取指令，将结果输出到标准输出。每条指令以换行分隔，输出结果的第一行附带时间戳 `[<timestamp>]`。

## 项目结构

```
Ticket-System-2026/
├── CMakeLists.txt                 # CMake 构建配置
├── management_system.md           # 管理系统需求规格说明
├── README.md                      # 本文件
└── src/
    ├── main.cpp                   # 程序入口，快速 I/O 与主循环
    ├── Grammar/
    │   ├── parser.cpp             # 指令解析器实现
    │   └── Token.cpp              # Token 定义与匹配表
    ├── include/
    │   ├── BPlusTree/
    │   │   ├── BPT.hpp            # B+ 树模板类（增删改查、迭代器、分裂/合并）
    │   │   ├── BPT_MemoryRiver.hpp # 基于文件的持久化存储引擎（含空闲链表）
    │   │   └── BufferPoolManager.hpp # ARC 自适应替换缓存池管理器
    │   ├── Grammar/
    │   │   ├── parser.hpp         # 指令解析器声明
    │   │   └── Token.hpp          # Token 类型与 TokenStream 定义
    │   ├── Library/
    │   │   ├── exceptions.hpp     # 异常类定义
    │   │   ├── list.hpp           # 手写双向链表（替代 std::list）
    │   │   ├── priority_queue.hpp # 手写优先队列
    │   │   ├── set.hpp            # 手写平衡树集合
    │   │   ├── string_key.hpp     # 定长字符串键（用于 BPT 索引）
    │   │   ├── unordered_map.hpp  # 手写哈希表（FNV 哈希，替代 std::unordered_map）
    │   │   ├── utility.hpp        # sjtu::pair（替代 std::pair）
    │   │   └── vector.hpp         # 手写动态数组（替代 std::vector）
    │   ├── Order/
    │   │   └── order.hpp          # 订单管理器（购票/退票/候补/查询）
    │   ├── Train/
    │   │   ├── comp.hpp           # 列车比较器
    │   │   ├── time.hpp           # 时间/日期类定义
    │   │   └── train.hpp          # 列车管理器（车次/票务/换乘查询）
    │   ├── User/
    │   │   └── user.hpp           # 用户管理器（注册/登录/权限/密码哈希）
    │   └── Validator/
    │       └── validator.hpp      # 输入校验器
    ├── Order/
    │   └── order.cpp              # 订单管理器实现
    ├── Train/
    │   └── train.cpp              # 列车管理器实现
    └── User/
        └── user.cpp               # 用户管理器实现
```

## 系统架构

### 整体设计

```
                        ┌──────────────────┐
                        │     main.cpp      │
                        │ 快速 I/O + 主循环  │
                        └────────┬─────────┘
                                 │ 创建并持有
                  ┌──────────────┼──────────────┐
                  │              │              │
          ┌───────▼──────┐ ┌────▼─────┐ ┌──────▼──────┐
          │    Parser    │ │  User    │ │   Train     │
          │   指令解析路由 │ │  Manager │ │  Manager    │
          └───────┬──────┘ └────┬─────┘ └──────┬──────┘
                  │             │              │
    根据命令类型分发到对应 Manager  │              │
    ┌─────────────┼─────────────┘              │
    │             │                            │
    │             │        ┌───────────────────┘
    │             │        │
    │     ┌───────▼────────▼───────┐
    │     │      OrderManager      │
    │     │  依赖 UserManager &    │
    │     │  TrainManager          │
    │     └───────────┬────────────┘
    │                 │
    ▼                 ▼
┌──────────────────────────────────────┐
│           持久化存储层                 │
│  BPT<Key, Value>  +  MemoryRiver<T>  │
│  +  BufferPoolManager<Page> (ARC)   │
└──────────────────────────────────────┘
```

### 核心数据结构

#### 1. B+ 树 (`BPT<Key, Value>`)

自实现的磁盘友好型 B+ 树，支持插入、删除、查找、修改、范围遍历操作。采用页式存储，每页大小 4096 字节，节点内使用二分查找定位。叶子节点通过 `next` 指针串联形成有序链表，支持高效范围扫描。

- **节点类型**：内部节点存储键与子节点索引，叶子节点存储键值对
- **分裂策略**：节点溢出时将其一分为二，将中间键上推到父节点；若父节点溢出则递归分裂
- **合并策略**：节点下溢时先尝试从相邻兄弟借一个键；若兄弟也不足则合并两节点并递归处理父节点
- **迭代器**：支持前向遍历，自动跨叶子节点跳转

#### 2. 持久化存储 (`MemoryRiver<T>`)

基于文件系统的定长对象存储引擎。每个对象通过 `int` 类型的位置索引访问,支持写入、读取、更新、删除操作。内部维护空闲链表复用已删除对象的空间，并在析构时将空闲链表持久化。

#### 3. ARC 缓存池 (`BufferPoolManager<Page>`)

实现了 Adaptive Replacement Cache（自适应替换缓存）算法，将缓存分为四个链表：

- **T1**：最近使用一次即被淘汰的页面（MRU）
- **T2**：多次访问被提升的常驻页面（MFU）
- **B1**：T1 淘汰页面的幽灵条目
- **B2**：T2 淘汰页面的幽灵条目

特性：
- **扫描抵抗**：对 Scan 类型访问设置窗口阈值（`K_SCAN_PROMOTION_THRESHOLD = 3`），连续扫描 3 次后才提升至 T2，避免顺序扫描污染缓存
- **自适应**：通过幽灵命中动态调整 T1/T2 的目标大小（参数 `p`）
- **根页面保护**：B+ 树的根节点常驻缓存，不会被淘汰

#### 4. 自实现容器库 (`Library/`)

由于课程限制只能使用 `iostream`、`string`、`cstdio`、`cmath`、`fstream`、`filesystem` 头文件，项目自行实现了所有必要的容器：

| 容器 | 文件 | 说明 |
|------|------|------|
| `sjtu::vector<T>` | `vector.hpp` | 动态数组，支持随机访问、插入、删除，容量倍增策略 |
| `sjtu::list<T>` | `list.hpp` | 双向链表，支持 O(1) 插入删除，支持 `splice` 操作 |
| `sjtu::unordered_map<K,V>` | `unordered_map.hpp` | 哈希表，FNV-1a 哈希，开放链地址法，自动 rehash |
| `sjtu::pair<T1,T2>` | `utility.hpp` | 二元组 |
| `sjtu::set<T>` | `set.hpp` | 有序集合 |
| `sjtu::priority_queue<T>` | `priority_queue.hpp` | 优先队列 |

### 功能模块

#### 用户管理 (`UserManager`)

- 用户名、密码（加盐哈希）、姓名、邮箱、权限等级
- 基于 B+ 树索引快速查找用户
- 登录状态维护（内部 `logset` 记录已登录用户）
- 权限校验：高权限用户可管理低权限用户

#### 列车管理 (`TrainManager`)

- 车次数据：车次号、站点序列、票价、发车时间、行车时间、停站时间、售票日期区间、列车类型、座位数
- **三级索引**：
  1. `trainIndex`：车次号 → 车次在文件中的地址
  2. `station_train_mapping`：站名 → 经过该站的所有车次
  3. `trainSegmentIndex`：(出发站, 到达站) → 包含该区间的车次段
- **座位管理**：`seat_manager` 按 (车次地址, 日期, 站序号) 索引，记录每站剩余票数
- **车票查询 (`query_ticket`)**：通过区间索引快速定位候选车次，按时间或票价排序
- **换乘查询 (`query_transfer`)**：在恰好换乘一次的约束下寻找最优方案，以总时间/总价格为主关键字、两趟车的 ID 为次级关键字排序

#### 订单管理 (`OrderManager`)

- 支持购票、退票、订单查询
- **候补队列**：当余票不足且用户选择候补（`-q true`)时，订单进入等待队列；退票时自动按时间顺序尝试满足候补订单
- 候补订单状态为 `[pending]`，补票成功后在查询时显示为 `[success]`
- 订单按交易时间从新到旧排序

## 指令说明

详细指令格式与返回值见 `management_system.md`。以下为简要索引：

| 指令 | 常用度 | 功能 |
|------|--------|------|
| `add_user` | N | 注册用户 |
| `login` | F | 用户登录 |
| `logout` | F | 用户登出 |
| `query_profile` | SF | 查询用户信息 |
| `modify_profile` | F | 修改用户信息 |
| `add_train` | N | 添加车次 |
| `delete_train` | N | 删除未发布车次 |
| `release_train` | N | 发布车次 |
| `query_train` | N | 查询车次详情 |
| `query_ticket` | SF | 查询车票 |
| `query_transfer` | N | 查询换乘方案 |
| `buy_ticket` | SF | 购票/候补 |
| `query_order` | F | 查询订单 |
| `refund_ticket` | N | 退票 |
| `clean` | R | 清除所有数据 |
| `exit` | R | 退出程序 |

## 性能设计要点

1. **零 STL 依赖**：所有容器均手工实现，保证完全控制内存布局和性能
2. **磁盘持久化**：所有数据通过 `MemoryRiver` 落盘，程序重启后数据不丢失
3. **B+ 树索引**：所有需要快速查找的数据均通过 B+ 树索引，单次操作 O(log N)
4. **ARC 缓存池**：利用访问模式自适应调整缓存策略，在有限内存下最大化命中率
5. **快速 I/O**：使用 `fread` 块读取输入 + 自定义输出缓冲区，减少系统调用开销
6. **紧凑内存布局**：定长字符数组代替 `std::string` 存储关键字段，减少内存碎片
7. **文件数量控制**：所有 B+ 树共享有限的数据文件，总量不超过 50 个文件的限制

## 技术亮点

- **自适应缓存替换 (ARC)**：结合 LRU 和 LFU 的优势，自动适应访问模式变化
- **扫描窗口机制**：避免大批量顺序扫描（如 `query_ticket` 遍历）将热点数据挤出缓存
- **FNV-1a 哈希**：字符串和整数的高质量哈希函数，哈希表装载因子 0.75 自动扩容
- **加盐密码哈希**：用户密码通过用户名派生盐值 + 自定义哈希函数存储，不存明文
- **候补队列**：退票时按时间顺序自动满足候补订单，保证公平性

## 许可

本项目为 SJTU CS1951 课程作业，仅供学习参考。