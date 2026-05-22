#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP
#include <string>
#include "../Grammar/Token.hpp"
#include "../Train/time.hpp"
class Validator
{
public:
    static bool validate_username(const std::string &username)
    {
        if (username.empty() || username.size() > 20)
            return false;
        if (!std::isalpha(username[0]))
            return false;
        for (char c: username)
        {
            if (!std::isalnum(c) && c != '_')
                return false;
        }
        return true;
    } // validate username format
    static bool validate_password(const std::string &password)
    {
        if (password.empty() || password.size() > 30)
            return false;
        for (char c: password)
        {
            if (!std::isprint(c))
                return false;
        }
        return true;
    } // validate password format

    static int utf8_char_len(unsigned char ch)
    {
        if ((ch & 0b10000000) == 0)
            return 1;
        // 0xxxxxxx → ASCII，1 byte character

        else if ((ch & 0b11100000) == 0b11000000)
            return 2;
        // 110xxxxx → 2 byte character

        else if ((ch & 0b11110000) == 0b11100000)
            return 3;
        // 1110xxxx → 3 byte character (most common for Chinese characters)

        else if ((ch & 0b11111000) == 0b11110000)
            return 4;
        // 11110xxx → 4 byte character (for some rare Chinese characters)

        else
            return -1;
        // invalid UTF-8 character
    }
    static bool is_chinese(int code)
    {
        return (code >= 0x4E00 && code <= 0x9FFF) || (code >= 0x3400 && code <= 0x4DBF) ||
               (code >= 0x20000 && code <= 0x2A6DF) || (code >= 0x2A700 && code <= 0x2B73F) ||
               (code >= 0x2B740 && code <= 0x2B81F) || (code >= 0x2B820 && code <= 0x2CEAF) ||
               (code >= 0x2CEB0 && code <= 0x2EBEF) || (code >= 0x30000 && code <= 0x3134F) ||
               (code >= 0x31350 && code <= 0x323AF);
    }
    static int chinese_count(const std::string &str)
    {
        int column = 0;
        int count = 0;
        while (column < str.size())
        {
            int char_len = utf8_char_len((unsigned char) str[column]);
            int start = 0;
            int code_point;
            switch (char_len)
            {
                case 1:
                case 2:
                    return -1; // one or two-byte characters are not allowed in name
                case 3: {
                    if (column + 2 >= str.size())
                        return -1;
                    unsigned char ch1 = str[column];
                    if (ch1 < 0xE0 || ch1 > 0xEF)
                        return -1;
                    unsigned char ch2 = str[column + 1];
                    if ((ch2 & 0b11000000) != 0b10000000)
                        return -1;
                    unsigned char ch3 = str[column + 2];
                    if ((ch3 & 0b11000000) != 0b10000000)
                        return -1;
                    int x = ch1 & 0b00001111;
                    int y = ch2 & 0b00111111;
                    int z = ch3 & 0b00111111;
                    code_point = x * 4096 + y * 64 + z;
                    if (code_point < 0x800)
                        return -1;
                    if (!is_chinese(code_point))
                        return -1;
                    column += 3;
                    break;
                }
                case 4: {
                    if (column + 3 >= str.size())
                        return -1;
                    unsigned char ch1 = str[column];
                    if (ch1 < 0xF0 || ch1 > 0xF4)
                        return -1;
                    unsigned char ch2 = str[column + 1];
                    if ((ch2 & 0b11000000) != 0b10000000)
                        return -1;
                    unsigned char ch3 = str[column + 2];
                    if ((ch3 & 0b11000000) != 0b10000000)
                        return -1;
                    unsigned char ch4 = str[column + 3];
                    if ((ch4 & 0b11000000) != 0b10000000)
                        return -1;
                    int x = ch1 & 0b00000111;
                    int y = ch2 & 0b00111111;
                    int z = ch3 & 0b00111111;
                    int w = ch4 & 0b00111111;
                    code_point = x * 262144 + y * 4096 + z * 64 + w;
                    if (code_point < 0x10000 || code_point > 0x10FFFF)
                        return -1;
                    if (!is_chinese(code_point))
                        return -1;
                    column += 4;
                    break;
                }
                default:
                    return -1; // invalid UTF-8 character
            }
            count++;
        }
        return count;
    }
    static bool validate_name(const std::string &name)
    {
        int count = chinese_count(name);
        return count >= 2 && count <= 5;
    } // validate name format (2 to 5 Chinese characters)
    /**
     * @brief 检查数字字符串是否满足条件
     * @param str 待检测字符串
     * @param type 需要验证的数字类型（如 TRAINID、SEATNUM 等）
     * @return true 如果字符串合法
     */
    static bool validate_number(const std::string &str, TokenType type) noexcept
    {
        if (str.empty() || str.size() > 6)
            return false;
        if (str[0] == '0' && str.size() > 1) // leading zeros are not allowed
            return false;
        for (char c: str)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
            {
                return false;
            }
        }
        int num = std::stoi(str);
        switch (type)
        {
            case PRIVILEGE:
                return num >= 0 && num <= 10; // Privilege level should be between 0 and 10
            case STATIONNUM:
                return num >= 2 && num <= 100; // Station number should be between 2 and 100
            case SEATNUM:
                return num >= 0 && num <= 100000; // Seat number should be between 1 and 100000
            case PRICES:
                return num >= 0 && num <= 100000; // Price should be between 1 and 100000
            case TRAVELTIMES:
                return num >= 0 && num <= 10000; // Travel time should be between 1 and 10000 minutes
            case STOPOVERTIMES:
                return num >= 0 && num <= 10000; // Stopover time should be between 1 and 10000 minutes
            default:
                return num >= 0; // For other types, just check if it's a valid non-negative integer
        }
    }

    /**
     * @brief 检查列车ID是否合法
     * @param trainID 列车ID字符串，由char范围内字符组成，长度不超过20
     * @return true 如果列车ID合法
     */
    static bool validate_trainid(const std::string &trainID)
    {
        if (trainID.empty() || trainID.size() > 20)
            return false;
        return true;
    }

    /**
     * @brief 检查单个站点名称是否合法
     * @param station 站点名称，由汉字组成，不超过10个汉字
     * @return true 如果站点名称合法
     */
    static bool validate_station(const std::string &station)
    {
        int count = chinese_count(station);
        return count >= 1 && count <= 10;
    }

    /**
     * @brief 检查列车类型是否合法
     * @param type 列车类型，一个大写字母
     * @return true 如果列车类型合法
     */
    static bool validate_type(const std::string &type)
    {
        return type.size() == 1 && std::isupper(static_cast<unsigned char>(type[0]));
    }

    static bool validate_time(const std::string &time, Time & start_time)
    {
        if (time.size() != 5 || time[2] != ':')
            return false;
        std::string hour_str = time.substr(0, 2);
        std::string minute_str = time.substr(3, 2);
        for (char c: hour_str)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
                return false;
        }
        for (char c: minute_str)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
                return false;
        }
        int hour = std::stoi(hour_str);
        int minute = std::stoi(minute_str);
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
            return false;
        start_time.hour = hour;
        start_time.minute = minute;
        return true;
    }

    static bool validate_saledate(const std::string & sale_date, Date & start_date, Date & end_date)
    {
        if (sale_date.size() != 11 || sale_date[5] != '|' || sale_date[2] != '-' || sale_date[8] != '-')
            return false;
        std::string start_date_str = sale_date.substr(0, 5);
        std::string end_date_str = sale_date.substr(6, 5);
        try
        {
            start_date = Date(start_date_str);
            end_date = Date(end_date_str);
        }
        catch (...)
        {
            return false;
        }
        if (end_date < start_date)
            return false;
        return true;
    }
};
#endif // VALIDATOR_HPP
