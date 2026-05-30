#include <cstdio>
#include <cstring>
#include <iostream>
#include "include/Grammar/parser.hpp"
#include "include/Order/order.hpp"
#include "include/Train/train.hpp"
#include "include/User/user.hpp"

char buf[1 << 20];
size_t p = 0, len = 0;

inline int gc()
{
    if (p == len)
    {
        len = fread(buf, 1, sizeof(buf), stdin);
        p = 0;
        if (len == 0)
            return EOF;
    }
    return buf[p++];
}
/**
 * @brief 程序入口
 *
 * 初始化解析器、用户管理器和日志模块
 * 逐行读取输入指令，并交给 Parser 执行
 */
int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    static char out_buf[131072];
    char *ptr = out_buf;
    char *const buf_end = out_buf + 131000;

    auto flush_output = [&]() {
        if (ptr != out_buf)
        {
            std::fwrite(out_buf, 1, static_cast<size_t>(ptr - out_buf), stdout);
            ptr = out_buf;
        }
    };

    auto append_char = [&](char ch) {
        if (ptr >= buf_end)
        {
            flush_output();
        }
        *ptr++ = ch;
    };

    auto append_string = [&](const std::string &s) {
        const char *data = s.data();
        size_t len = s.size();
        while (len > 0)
        {
            if (ptr >= buf_end)
            {
                flush_output();
            }
            size_t cap = static_cast<size_t>(buf_end - ptr);
            size_t chunk = (len < cap) ? len : cap;
            std::memcpy(ptr, data, chunk);
            ptr += chunk;
            data += chunk;
            len -= chunk;
        }
    };

    // fast input: read lines using fread
    auto fast_getline = [](std::string &s) -> bool {
        s.clear();
        int c = gc();
        if (c == EOF)
            return false;
        while (c != '\n' && c != EOF)
        {
            if (c != '\r')
                s.push_back(char(c));
            c = gc();
        }
        return true;
    };
    Parser parser; // 指令解析器
    UserManager userManager; // 用户管理器
    OrderManager orderManager(userManager); // 订单管理器
    TrainManager trainManager; // 列车管理器
    std::string line; // 存储用户输入的一行指令

    // 循环读取标准输入的每一行
    while (fast_getline(line))
    {
        bool is_running = true;

        std::string output = parser.execute(line, userManager, trainManager, orderManager, is_running);
        if (!output.empty())
        {
            append_string(output);
            append_char('\n');
        }
        if (!is_running)
        {
            flush_output();
            return 0;
        }
    }
    flush_output();
    return 0;
}
