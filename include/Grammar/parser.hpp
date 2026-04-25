#pragma once
#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include "../Library/unordered_map.hpp"
#include "../User/user.hpp"
#include "token.hpp"


enum class ParserState
{
    COMMAND,
    USER,
    TRAIN
};
class Parser
{
public:
    inline static sjtu::unordered_map<TokenType> CMDTABLE = {
            {"login", LOGIN},
            {"logout", LOGOUT},
            {"add_user", ADDUSER},
            {"query_profile", QUERYPROFILE},
            {"modify_profile", MODIFYPROFILE},
            {"add_train", ADDTRAIN},
            {"delete_train", DELETETRAIN},
            {"release_train", RELEASETRAIN},
            {"query_train", QUERYTRAIN},
            {"query_ticket", QUERYTICKET},
            {"query_transfer", QUERYTRANSFER},
            {"refund_ticket", REFUNDTICKET},
            {"query_order", QUERYORDER},
            {"buy_ticket", BUYTICKET},
            {"clean", CLEAN},
            {"exit", EXIT},
    }; // 构建从指令字符串到枚举类的一个映射
    inline static sjtu::unordered_map<TokenType> USERTABLE = {{"-c", CURUSERNAME}, {"-u", USERNAME},
                                                              {"-p", PASSWORD},    {"-n", NAME},
                                                              {"-m", MAILADDRESS}, {"-g", PRIVILEGE}};
    // 构建从用户参数字符串到枚举类的一个映射

    inline static ParserState State(TokenType type)
    {
        switch (type)
        {
            case LOGIN:
            case LOGOUT:
            case ADDUSER:
            case QUERYPROFILE:
            case MODIFYPROFILE:
                return ParserState::USER;
            case ADDTRAIN:
            case DELETETRAIN:
            case RELEASETRAIN:
            case QUERYTRAIN:
            case QUERYTICKET:
            case QUERYTRANSFER:
            case REFUNDTICKET:
            case QUERYORDER:
            case BUYTICKET:
                return ParserState::TRAIN;
            default:
                return ParserState::COMMAND; // 默认返回 COMMAND 状态
        }
    }
    /**
     * @brief 将一行指令拆分为 TokenStream
     * @param result 存储时间戳的字符串
     * @param line 输入指令行
     * @return 解析后的 TokenStream
     */
    TokenStream tokenize(std::string &result, const std::string &line) const;


    /**
     * @brief 将关键字字符串匹配到 TokenType 枚举
     * @param state 当前解析状态
     * @param text 输入的字符串
     * @return 匹配的 TokenType，如果未匹配返回 BLANK
     */
    TokenType matchkeyword(ParserState state, const std::string &text) const;

    /**
     * @brief 执行解析和指令处理
     * @param line 输入指令
     * @param userManager 用户管理器对象
     * @param is_running 程序运行状态标志
     * @return 执行结果字符串（如有需要），否则返回空字符串
     */
    std::string execute(const std::string &, UserManager &, bool &is_running);
};
#endif
