#include "../include/Grammar/parser.hpp"
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
    if (state == ParserState::TRAINCMD)
    {
        auto it = TRAINCOMMANDTABLE.find(text);
        if (it != TRAINCOMMANDTABLE.end())
            return it->second;
        return BLANK;
    }
    if (state == ParserState::TICKETCMD)
    {
        auto it = TICKETCOMMANDTABLE.find(text);
        if (it != TICKETCOMMANDTABLE.end())
            return it->second;
        return BLANK;
    }
    return BLANK;
}

// 解析一行输入，将输入行分割成 tokens
TokenStream Parser::tokenize(std::string &result, const std::string &line_raw, int &time_stamp) const
{
    // 支持行首可选的时间戳格式 [<timestamp>]
    std::string line = line_raw;
    if (!line.empty() && line.front() == '[')
    {
        auto pos = line.find(']');
        if (pos != std::string::npos)
        {
            result = line.substr(0, pos + 1); // 提取时间戳
            time_stamp = std::stoi(line.substr(1, pos - 1)); // 将时间戳字符串转换为整数
            result += " "; // 格式化处理时间戳
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
std::string Parser::execute(const std::string &line_raw, UserManager &userManager, TrainManager &trainManager,
                            OrderManager &orderManager, bool &is_running)
{
    std::string line = line_raw;
    std::string result;
    if (line.empty())
        return "";
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.pop_back();

    int time_stamp; // 可选的时间戳处理
    TokenStream tokens_ = tokenize(result, line, time_stamp);
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

    else if (State(cmd) == ParserState::TRAINCMD || cmd == QUERYTRAIN || cmd == QUERYTICKET || cmd == QUERYTRANSFER)
    {
        // 处理列车相关指令
        result += trainManager.handleTrainCommand(tokens_);
    }
    else if (State(cmd) == ParserState::TICKETCMD)
    {
        // 处理购票相关指令
        result += orderManager.handleOrderCommand(tokens_, trainManager, time_stamp);
    }
    else if (cmd == CLEAN)
    {
        // 处理清除数据指令
        userManager.clean();
        trainManager.clean();
        orderManager.clean();
        result += "0";
    }
    else if (cmd == EXIT)
    {
        // 处理退出指令
        userManager.exit();
        is_running = false;
        result += "bye";
    }

    return result;
}

// 静态成员定义和静态表初始化结构体
sjtu::unordered_map<std::string, TokenType> Parser::CMDTABLE;
sjtu::unordered_map<std::string, TokenType> Parser::USERTABLE;
sjtu::unordered_map<std::string, TokenType> Parser::TRAINCOMMANDTABLE;
sjtu::unordered_map<std::string, TokenType> Parser::TICKETCOMMANDTABLE;
struct ParserStaticInit
{
    ParserStaticInit()
    {
        Parser::CMDTABLE.insert({"login", LOGIN});
        Parser::CMDTABLE.insert({"logout", LOGOUT});
        Parser::CMDTABLE.insert({"add_user", ADDUSER});
        Parser::CMDTABLE.insert({"query_profile", QUERYPROFILE});
        Parser::CMDTABLE.insert({"modify_profile", MODIFYPROFILE});
        Parser::CMDTABLE.insert({"add_train", ADDTRAIN});
        Parser::CMDTABLE.insert({"delete_train", DELETETRAIN});
        Parser::CMDTABLE.insert({"release_train", RELEASETRAIN});
        Parser::CMDTABLE.insert({"query_train", QUERYTRAIN});
        Parser::CMDTABLE.insert({"query_ticket", QUERYTICKET});
        Parser::CMDTABLE.insert({"query_transfer", QUERYTRANSFER});
        Parser::CMDTABLE.insert({"refund_ticket", REFUNDTICKET});
        Parser::CMDTABLE.insert({"query_order", QUERYORDER});
        Parser::CMDTABLE.insert({"buy_ticket", BUYTICKET});
        Parser::CMDTABLE.insert({"clean", CLEAN});
        Parser::CMDTABLE.insert({"exit", EXIT});

        Parser::USERTABLE.insert({"-c", CURUSERNAME});
        Parser::USERTABLE.insert({"-u", USERNAME});
        Parser::USERTABLE.insert({"-p", PASSWORD});
        Parser::USERTABLE.insert({"-n", NAME});
        Parser::USERTABLE.insert({"-m", MAILADDRESS});
        Parser::USERTABLE.insert({"-g", PRIVILEGE});

        Parser::TRAINCOMMANDTABLE.insert({"-i", TRAINID});
        Parser::TRAINCOMMANDTABLE.insert({"-n", STATIONNUM});
        Parser::TRAINCOMMANDTABLE.insert({"-m", SEATNUM});
        Parser::TRAINCOMMANDTABLE.insert({"-s", STATIONS});
        Parser::TRAINCOMMANDTABLE.insert({"-p", PRICES});
        Parser::TRAINCOMMANDTABLE.insert({"-x", STARTTIME});
        Parser::TRAINCOMMANDTABLE.insert({"-t", TRAVELTIMES});
        Parser::TRAINCOMMANDTABLE.insert({"-o", STOPOVERTIMES});
        Parser::TRAINCOMMANDTABLE.insert({"-d", SALEDATE});
        Parser::TRAINCOMMANDTABLE.insert({"-y", TRAINTYPE});

        Parser::TICKETCOMMANDTABLE.insert({"-u", USERNAME});
        Parser::TICKETCOMMANDTABLE.insert({"-i", TRAINID});
        Parser::TICKETCOMMANDTABLE.insert({"-d", QUERYDATE});
        Parser::TICKETCOMMANDTABLE.insert({"-s", STARTPLACE});
        Parser::TICKETCOMMANDTABLE.insert({"-t", DESTINATION});
        Parser::TICKETCOMMANDTABLE.insert({"-q", INQUEUE});
        Parser::TICKETCOMMANDTABLE.insert({"-n", NUMBER});
        Parser::TICKETCOMMANDTABLE.insert({"-f", STARTPLACE});
        Parser::TICKETCOMMANDTABLE.insert({"-p", PRIORITY});
    }
};
static ParserStaticInit parser_static_init_instance;
