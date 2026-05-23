#pragma once
#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include "../Library/unordered_map.hpp"
#include "../Train/train.hpp"
#include "../User/user.hpp"
#include "../Order/order.hpp"
#include "Token.hpp"


enum class ParserState
{
    COMMAND,
    USER,
    TRAINCMD,
    TICKETCMD,
};
class Parser
{
public:
    static sjtu::unordered_map<std::string, TokenType> CMDTABLE;
    static sjtu::unordered_map<std::string, TokenType> USERTABLE;
    static sjtu::unordered_map<std::string, TokenType> TRAINCOMMANDTABLE;
    static sjtu::unordered_map<std::string, TokenType> TICKETCOMMANDTABLE;
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
                return ParserState::TRAINCMD;
            case QUERYTRAIN:
            case QUERYTICKET:
            case QUERYTRANSFER:
            case REFUNDTICKET:
            case QUERYORDER:
            case BUYTICKET:
                return ParserState::TICKETCMD;
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
    TokenStream tokenize(std::string &result, const std::string &line_raw, int &time_stamp) const;


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
    std::string execute(const std::string &, UserManager &, TrainManager &, OrderManager &, bool &is_running);
};
#endif
