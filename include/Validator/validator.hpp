#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP
#include <string>
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
};
#endif // VALIDATOR_HPP
