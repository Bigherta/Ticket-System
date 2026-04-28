#include <iostream>
#include "../include/Grammar/parser.hpp"
#include "../include/User/user.hpp"
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
    Parser parser; // 指令解析器
    UserManager userManager; // 用户管理器
    std::string line; // 存储用户输入的一行指令

    // 循环读取标准输入的每一行
    while (std::getline(std::cin, line))
    {
        bool is_running = true;

        std::string output = parser.execute(line, userManager, is_running);
        if (!output.empty())
        {
            std::cout << output << '\n';
        }
        if (!is_running || std::cin.eof())
        {
            return 0;
        }
    }
    return 0;
}
