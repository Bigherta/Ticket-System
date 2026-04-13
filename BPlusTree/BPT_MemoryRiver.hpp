#ifndef BPT_MEMORYRIVER_HPP
#define BPT_MEMORYRIVER_HPP

#include <fstream>
#include "vector.hpp"

using std::fstream;
using std::ifstream;
using std::ofstream;
using std::string;

template<class T, int info_len = 2>
class MemoryRiver
{
private:
    /* your code here */
    fstream file;
    fstream free_file;
    string file_name;
    int sizeofT = sizeof(T);
    sjtu::vector<int> free_index;
    std::string free_block_name() { return file_name + ".free"; }

public:
    MemoryRiver() = default;

    MemoryRiver(const string &file_name) : file_name(file_name) {}

    ~MemoryRiver()
    {
        if (file.is_open())
            file.close();
        free_file.open(free_block_name(), std::ios::out | std::ios::trunc | std::ios::binary);
        for (auto &index: free_index)
        {
            free_file.write(reinterpret_cast<char *>(&index), sizeof(int));
        }
    }

    void initialise(string FN = "")
    {
        if (!FN.empty())
            file_name = FN;

        free_index.clear();

        // Open data file. If it does not exist, create it and initialize info area.
        file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            file.open(file_name, std::ios::out | std::ios::binary);
            int tmp = 0;
            for (int i = 0; i < info_len; ++i)
                file.write(reinterpret_cast<char *>(&tmp), sizeof(int));
            file.close();
            file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        }

        // Open free-list file. If it does not exist, create it first.
        free_file.open(free_block_name(), std::ios::in | std::ios::out | std::ios::binary);
        if (!free_file.is_open())
        {
            free_file.open(free_block_name(), std::ios::out | std::ios::binary);
            free_file.close();
            free_file.open(free_block_name(), std::ios::in | std::ios::out | std::ios::binary);
        }

        free_file.seekg(0, std::ios::end);
        int free_file_end = free_file.tellg();
        free_file.seekg(0, std::ios::beg);
        while (free_file.tellg() + static_cast<std::streamoff>(sizeof(int)) <= free_file_end)
        {
            int free_pos;
            free_file.read(reinterpret_cast<char *>(&free_pos), sizeof(int));
            free_index.push_back(free_pos);
        }

        file.close();
        free_file.close();
    }

    // 读出第n个int的值赋给tmp，1_base
    void get_info(int &tmp, int n)
    {
        if (n > info_len)
            return;
        /* your code here */
        if (!file.is_open())
        {
            file.open(file_name, std::ios::out | std::ios::in | std::ios::binary);
        }
        file.seekg((n - 1) * sizeof(int));
        file.read(reinterpret_cast<char *>(&tmp), sizeof(int));
        file.close();
    }

    // 将tmp写入第n个int的位置，1_base
    void write_info(int tmp, int n)
    {
        if (n > info_len)
            return;
        /* your code here */
        if (!file.is_open())
        {
            file.open(file_name, std::ios::out | std::ios::in | std::ios::binary);
        }
        file.seekp((n - 1) * sizeof(int));
        file.write(reinterpret_cast<char *>(&tmp), sizeof(int));
        file.close();
    }

    // 在文件合适位置写入类对象t，并返回写入的位置索引index
    // 位置索引意味着当输入正确的位置索引index，在以下三个函数中都能顺利的找到目标对象进行操作
    // 位置索引index可以取为对象写入的起始位置
    int write(T &t)
    {
        /* your code here */
        if (!file.is_open())
        {
            file.open(file_name, std::ios::out | std::ios::in | std::ios::binary);
        }
        int index;
        if (!free_index.empty())
        {
            file.seekp(free_index.back());
            free_index.pop_back();
            index = file.tellp();
            file.write(reinterpret_cast<char *>(&t), sizeofT);
        }
        else
        {
            file.seekp(0, std::ios::end);
            index = file.tellp();
            file.write(reinterpret_cast<char *>(&t), sizeofT);
        }
        file.close();
        return index;
    }

    // 用t的值更新位置索引index对应的对象，保证调用的index都是由write函数产生
    void update(T &t, const int index)
    {
        /* your code here */
        if (!file.is_open())
        {
            file.open(file_name, std::ios::out | std::ios::in | std::ios::binary);
        }
        file.seekp(index);
        file.write(reinterpret_cast<char *>(&t), sizeofT);
        file.close();
    }

    // 读出位置索引index对应的T对象的值并赋值给t，保证调用的index都是由write函数产生
    void read(T &t, const int index)
    {
        /* your code here */
        if (!file.is_open())
        {
            file.open(file_name, std::ios::out | std::ios::in | std::ios::binary);
        }
        file.seekg(index);
        file.read(reinterpret_cast<char *>(&t), sizeofT);
        file.close();
    }

    // 删除位置索引index对应的对象，保证调用的index都是由write函数产生
    void Delete(int index) { /* your code here */ free_index.push_back(index); }
};


#endif // BPT_MEMORYRIVER_HPP