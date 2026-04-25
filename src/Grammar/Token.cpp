#include "../../include/Grammar/Token.hpp"

TokenStream::TokenStream(sjtu::vector<Token> &&tokens) : tokens_(std::move(tokens)), cursor_(0) {}

const Token* TokenStream::peek() const
{
    if (cursor_ >= tokens_.size())
    {
        return nullptr;
    }
    const Token* temp = &tokens_[cursor_];
    return temp;
} // 查看当前token并返回指针

const Token* TokenStream::get()
{
    const Token* current = peek();
    if (current != nullptr)
    {
        ++cursor_;
    }
    return current;
} // 取出当前token并前进

int TokenStream::size() const { return tokens_.size(); } // 总数

void TokenStream::push(Token &&token) { tokens_.push_back(std::move(token)); } // 输入token
