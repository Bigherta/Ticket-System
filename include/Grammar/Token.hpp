#pragma once
#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>
#include "../Library/vector.hpp"

/**
 * @enum TokenType
 * @brief 枚举指令或关键字类型，用于解析用户输入
 */
enum TokenType
{
    LOGIN, ///< 登录指令
    LOGOUT, ///< 登出指令
    ADDUSER, ///< 注册指令
    MODIFYPROFILE, ///< 修改用户信息指令
    QUERYPROFILE, ///< 查询用户信息指令

    CURUSERNAME, ///< 当前用户名参数
    USERNAME, ///< 用户名参数
    PASSWORD, ///< 密码参数
    NAME, ///< 姓名参数
    MAILADDRESS, ///< 邮箱地址参数
    PRIVILEGE, ///< 权限参数

    ADDTRAIN, ///< 添加火车指令
    DELETETRAIN, ///< 删除火车指令
    RELEASETRAIN, ///< 发布火车指令
    QUERYTRAIN, ///< 查询火车指令
    QUERYTICKET, ///< 查询车票指令
    QUERYTRANSFER, ///< 查询换乘指令
    BUYTICKET, ///< 购买车票指令
    REFUNDTICKET, ///< 退票指令
    QUERYORDER, ///< 查询订单指令

    TRAINID, ///< 火车编号参数
    TRAINTYPE, ///< 火车类型参数
    STATIONNUM, ///< 车站数量参数
    SEATNUM, ///< 座位数量参数
    STATIONS, ///< 车站参数
    PRICES, ///< 价格参数
    STARTTIME, ///< 出发时间参数
    TRAVELTIMES, ///< 旅行时间参数
    STOPOVERTIMES, ///< 停车时间参数
    SALEDATE, ///< 发售日期参数

    CLEAN, ///< 清除数据指令
    EXIT, ///< 退出指令
    BLANK ///< 空白或无效输入
};

/**
 * @struct Token
 * @brief 封装词法分析得到的基本单元
 */
struct Token
{
    TokenType type{TokenType::BLANK}; ///< Token 类型
    std::string text{}; ///< 原始文本内容
    int column{0}; ///< 在输入行中的列位置
};

/**
 * @class TokenStream
 * @brief 封装 Token 流，用于指令解析
 */
class TokenStream
{
public:
    /**
     * @brief 默认构造函数
     */
    TokenStream() = default;

    /**
     * @brief 构造函数，使用移动语义初始化 Token 流
     * @param tokens 需要初始化的 Token 向量
     */
    explicit TokenStream(sjtu::vector<Token> &&tokens);

    /**
     * @brief 查看当前 Token，不移动游标
     * @return std::shared_ptr<Token> 当前 Token 指针
     */
    const Token *peek() const;

    /**
     * @brief 获取当前 Token，并移动游标到下一个
     * @return std::shared_ptr<Token> 当前 Token 指针
     */
    const Token *get();

    /**
     * @brief 获取 Token 流的大小
     * @return int Token 数量
     */
    int size() const;

    /**
     * @brief 向 Token 流中推入新的 Token
     * @param token 要添加的 Token
     */
    void push(Token &&token);

private:
    sjtu::vector<Token> tokens_{}; ///< 存储 Token 的向量
    int cursor_{0}; ///< 当前游标位置
};

#endif // TOKEN_HPP
