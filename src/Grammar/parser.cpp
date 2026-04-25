#include "../../include/Grammar/parser.hpp"
// 将关键字根据解析状态匹配成对应的枚举类
TokenType Parser::matchkeyword(ParserState state, const std::string &text) const
{
    if (state == ParserState::COMMAND)
    {
        auto it = CMDTABLE.find(text);
        if (it != CMDTABLE.end())
            return it->second;
        return BLANK;
    }
    if (state == ParserState::USER)
    {
        auto it = USERTABLE.find(text);
        if (it != USERTABLE.end())
            return it->second;
        return BLANK;
    }
    return BLANK;
}

// 解析一行输入，将输入行分割成 tokens
TokenStream Parser::tokenize(std::string &result, const std::string &line_raw) const
{
    // 支持行首可选的时间戳格式 [<timestamp>]
    std::string line = line_raw;
    if (!line.empty() && line.front() == '[')
    {
        auto pos = line.find(']');
        if (pos != std::string::npos)
        {
            result = line.substr(0, pos + 1); // 提取时间戳
            line = line.substr(pos + 1);
        }
    }

    sjtu::vector<Token> tokens;

    // extract the command token (the first non-space word)
    size_t i = 0;
    // skip spaces
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
        ++i;
    // parse a token
    size_t start = i;
    while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i])))
        ++i;
    std::string word = line.substr(start, i - start);
    TokenType type = matchkeyword(ParserState::COMMAND, word);
    tokens.push_back(Token{type, word});
    
    // determine parsing state based on the command token
    ParserState state = State(type);

    // 继续解析剩余的参数
    while (i < line.size())
    {
        // skip spaces
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        if (i >= line.size())
            break;
        // parse a parameter token
        start = i;
        while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        word = line.substr(start, i - start);
        type = matchkeyword(state, word);

        // skip spaces between key and value
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
            ++i;

        // parse a value token (the next non-space word)
        start = i;
        while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        word = line.substr(start, i - start);
        tokens.push_back(Token{type, word});
    }
    return TokenStream(std::move(tokens));
}

// 执行指令的主函数
std::string Parser::execute(const std::string &line_raw, UserManager &userManager, bool &is_running)
{
    std::string line = line_raw;
    std::string result;
    if (line.empty())
        return "";
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.pop_back();

    TokenStream tokens_ = tokenize(result, line);
    if (tokens_.size() == 0)
        return "";

    // 解析并处理用户相关指令
    const Token *first = tokens_.peek();
    if (first == nullptr)
        return "";
    TokenType cmd = first->type;

    if (State(cmd) == ParserState::USER)
    {
        // 处理用户相关指令
        result += userManager.handleUserCommand(tokens_);
    }
    return result;
}
