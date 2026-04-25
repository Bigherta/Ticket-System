#ifndef USER_HPP
#define USER_HPP
#include <string>
#include "../BPlusTree/BPT.hpp"
#include "../Library/set.hpp"
class UserManager;
class User
{
    friend class UserManager;
private:
    char username[21]{};
    unsigned long long password{};
    unsigned long long salt{};
    char name[21]{};
    char mailAddr[31]{};
    int privilege{};

public:
    User() = default;
    static unsigned long long generate_salt(std::string username)
    {
        unsigned long long current_salt = 0;
        for (int i = 0; username[i]; i++)
        {
            current_salt ^= (unsigned long long) (unsigned char) username[i];
            current_salt *= 1099511628211ULL;
            current_salt ^= (current_salt << 13);
            current_salt ^= (current_salt >> 7);
            current_salt ^= (current_salt << 17);
        }
        return current_salt;
    } // generate a unique salt for each user
    static unsigned long long hash_password(std::string password, unsigned long long salt)
    {
        unsigned long long h = salt ^ 1469598103934665603ULL;

        for (int i = 0; password[i]; i++)
        {
            h ^= (unsigned long long) (unsigned char) password[i];
            h *= 1099511628211ULL;
            h ^= (h << 13);
            h ^= (h >> 7);
            h ^= (h << 17);
        }
        return h;
    } // hash function for password
    User(const std::string &username_, const std::string &password_, const std::string &name_,
         const std::string &mailAddr_, int privilegeLevel_ = 0) : privilege(privilegeLevel_)
    {
        std::snprintf(username, sizeof(username), "%s", username_.c_str());
        std::snprintf(name, sizeof(name), "%s", name_.c_str());
        std::snprintf(mailAddr, sizeof(mailAddr), "%s", mailAddr_.c_str());
        salt = generate_salt(username_);
        password = hash_password(password_, salt);
    }

    std::string to_string() const
    {
        return std::string(username) + " " + std::string(name) + " " + std::string(mailAddr) + " " + std::to_string(privilege);
    }
};
class UserManager
{
public:
    /**
     * @brief 构造函数，初始化 UserManager 对象
     */
    UserManager();
    /**
     * @brief 用户注册
     * @param cur_User 当前操作用户的用户名，创建第一个用户时忽略该参数
     * @param username_ 注册用户名
     * @param password_ 用户密码
     * @param name_ 用户姓名
     * @param mailAddr_ 用户邮箱地址
     * @param privilegeLevel_ 用户权限等级，创建第一个用户时忽略该参数
     * @return true 注册成功
     * @return false 注册失败（用户ID已存在或参数非法）
     */
    int registerUser(const std::string &cur_User, const std::string &username_, const std::string &password_,
                      const std::string &name_, const std::string &mailAddr_, int privilegeLevel_);

    /**
     * @brief 检查指定用户是否已登录
     * @param username_ 要检查的用户名
     * @return true 如果用户已登录
     * @return false 如果用户未登录
     */
    bool is_log(const std::string &username_);

    /**
     * @brief 用户登录操作
     * @param username_ 用户名
     * @param password_ 用户密码
     * @return true 登录成功
     * @return false 登录失败（用户不存在或密码错误）
     */
    int login(const std::string &username_, const std::string &password_);

    /**
     * @brief 用户登出操作
     * @param username_ 用户名
     * @return true 登出成功
     * @return false 当前没有用户登录
     */
    int logout(const std::string &username_);

    /**
     * @brief 访问用户信息
     * @param cur_User 当前操作用户的用户名
     * @param username_ 要访问的用户名
     * @return 用户信息（若访问失败，则返回-1）
     */
    std::string queryUser(const std::string &cur_User, const std::string &username_);

    /**
     * @brief 修改用户信息
     * @param cur_User 当前操作用户的用户名
     * @param username_ 要修改的用户名
     * @param name_ 新姓名（可选， 传入空字符串表示不修改）
     * @param password_ 新密码（可选， 传入空字符串表示不修改）
     * @param mailAddr_ 新邮箱地址（可选， 传入空字符串表示不修改）
     * @param privilegeLevel_ 新权限等级（可选， 传入-1表示不修改）
     * @return 用户信息（若修改失败，则返回-1）
     */
    std::string modifyUser(const std::string &cur_User, const std::string &username_, const std::string &name_,
                           const std::string &password_, const std::string &mailAddr_, int privilegeLevel_);

    /**
     * @brief 退出系统。
     */
    void exit();

    /**
     * @brief 删除用户数据。
     */
    void clean();

private:
    struct UserName
    {
        char name[21]{};
        UserName(const std::string &name_)
        {
            std::snprintf(name, sizeof(name), "%s", name_.c_str());
        }
        bool operator<(const UserName &other) const { return std::strcmp(name, other.name) < 0; }
        bool operator==(const UserName &other) const { return std::strcmp(name, other.name) == 0; }
        bool operator!=(const UserName &other) const { return !(*this == other); }
        bool operator<=(const UserName &other) const { return std::strcmp(name, other.name) <= 0; }
    };
    /// 用户索引库，使用 BPT 存储 username 和对应的 User 对象在文件中的位置
    BPT<UserName> userIndex;
    /// 用户数据库， 使用MemoryRiver存储 User 对象
    MemoryRiver<User> userDatabase;
    /// 用户操作记录集合，每条记录为username
    sjtu::set<std::string> logset;
};
#endif // USER_HPP
