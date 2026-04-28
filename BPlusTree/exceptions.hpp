#ifndef SJTU_EXCEPTIONS_HPP
#define SJTU_EXCEPTIONS_HPP

#include <cstring>
#include <string>

namespace sjtu
{

    class exception
    {
    protected:
        std::string detail = "";

    public:
        exception() {}
        exception(const exception &ec) : detail(ec.detail) {}
        exception(const std::string &det) : detail(det) {}
        virtual std::string what() { return detail; }
    };

    class index_out_of_bound : public exception
    {
    public:
        index_out_of_bound() {}
        index_out_of_bound(const std::string &det) : exception(det) {}
    };
    class runtime_error : public exception
    {
    public:
        runtime_error() {}
        runtime_error(const std::string &det) : exception(det) {}
    };
    class invalid_iterator : public exception
    {
    public:
        invalid_iterator() {}
        invalid_iterator(const std::string &det) : exception(det) {}
    };

    class container_is_empty : public exception
    {
    public:
        container_is_empty() {}
        container_is_empty(const std::string &det) : exception(det) {}
    };
} // namespace sjtu

#endif
