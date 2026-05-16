#include "exceptions.hpp"
template <typename T>
class shared_ptr {
private:
    struct control {
        T *data = nullptr;
        int ref_count = 1;
        control(T *p) : data(p), ref_count(1) {}
    };
    control *value = nullptr;

public:
    shared_ptr() = default;

    explicit shared_ptr(T *ptr) { value = new control(ptr); }

    shared_ptr(const shared_ptr &other) { value = other.value; if (value) value->ref_count++; }

    shared_ptr &operator=(const shared_ptr &other) {
        if (this == &other) return *this;
        if (value) {
            if (--value->ref_count == 0) {
                delete value->data;
                delete value;
            }
        }
        value = other.value;
        if (value) value->ref_count++;
        return *this;
    }

    void reset(T *ptr) {
        if (value) {
            if (--value->ref_count == 0) {
                delete value->data;
                delete value;
            }
        }
        value = new control(ptr);
    }

    void reset() {
        if (value) {
            if (--value->ref_count == 0) {
                delete value->data;
                delete value;
            }
            value = nullptr;
        }
    }

    T &operator*() const {
        if (value && value->data) return *value->data;
        throw sjtu::exception();
    }

    T *operator->() const {
        if (value && value->data) return value->data;
        throw sjtu::exception();
    }

    ~shared_ptr() {
        if (value) {
            if (--value->ref_count == 0) {
                delete value->data;
                delete value;
            }
            value = nullptr;
        }
    }
};