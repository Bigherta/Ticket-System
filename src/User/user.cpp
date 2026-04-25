#include "../../include/User/user.hpp"
#include "../../include/Validator/validator.hpp"
UserManager::UserManager() : userIndex("userIndex.dat") { userDatabase.initialise("userDatabase.dat"); }

int UserManager::registerUser(const std::string &cur_User, const std::string &username_, const std::string &password_,
                              const std::string &name_, const std::string &mailAddr_, int privilegeLevel_)
{
    if (!Validator::validate_username(username_) || !Validator::validate_password(password_) ||
        !Validator::validate_name(name_))
        return -1; // 参数非法
    if (userIndex.empty())
    {
        User newUser(username_, password_, name_, mailAddr_, 10);
        int addr = userDatabase.write(newUser);
        userIndex.insert(UserName{username_}, addr);
    }
    else
    {
        if (!Validator::validate_username(cur_User) || !is_log(cur_User))
            return -1; // 当前用户未登录或用户名非法
        if (userIndex.count(UserName{username_}))
            return -1; // 用户名已存在
        User curUser;
        userDatabase.read(curUser, userIndex.visit(UserName{cur_User}));
        if (curUser.privilege <= privilegeLevel_)
            return -1; // 当前用户权限不足
        User newUser(username_, password_, name_, mailAddr_, privilegeLevel_);
        int addr = userDatabase.write(newUser);
        userIndex.insert(UserName{username_}, addr);
    }
    return 0;
}

bool UserManager::is_log(const std::string &username_) { return logset.find(username_) != logset.end(); }

int UserManager::login(const std::string &username_, const std::string &password_)
{
    if (!Validator::validate_username(username_) || !Validator::validate_password(password_))
        return -1; // 参数非法
    if (is_log(username_))
        return -1; // 用户已登录
    if (!userIndex.count(UserName{username_}))
        return -1; // 用户不存在
    User user;
    userDatabase.read(user, userIndex.visit(UserName{username_}));
    if (User::hash_password(password_, user.salt) != user.password)
        return -1; // 密码错误
    logset.emplace(username_);
    return 0;
}

int UserManager::logout(const std::string &username_)
{
    if (!Validator::validate_username(username_))
        return -1; // 参数非法
    auto it = logset.find(username_);
    if (it == logset.end())
        return -1; // 用户未登录
    logset.erase(it);
    return 0;
}

std::string UserManager::queryUser(const std::string &cur_User, const std::string &username_)
{
    if (!Validator::validate_username(cur_User) || !is_log(cur_User))
        return "-1"; // 当前用户未登录或用户名非法
    auto target_index = userIndex.visit(UserName{username_});
    if (target_index == -1)
        return "-1"; // 用户不存在
    User curUser;
    userDatabase.read(curUser, userIndex.visit(UserName{cur_User}));
    User targetUser;
    userDatabase.read(targetUser, target_index);
    // 允许查询的条件：操作者是目标用户本人，或操作者权限严格大于目标用户
    if (cur_User != username_ && curUser.privilege <= targetUser.privilege)
        return "-1"; // 当前用户权限不足
    return targetUser.to_string();
}

std::string UserManager::modifyUser(const std::string &cur_User, const std::string &username_,
                                    const std::string &password_, const std::string &name_,
                                    const std::string &mailAddr_, int privilegeLevel_ = -1)
{
    if (!Validator::validate_username(cur_User) || !is_log(cur_User))
        return "-1"; // 当前用户未登录或用户名非法
    auto target_index = userIndex.visit(UserName{username_});
    if (target_index == -1)
        return "-1"; // 用户不存在
    User curUser;
    userDatabase.read(curUser, userIndex.visit(UserName{cur_User}));
    User targetUser;
    userDatabase.read(targetUser, target_index);

    // 权限：操作者必须是目标用户本人，或操作者权限严格大于目标用户
    if (cur_User != username_ && curUser.privilege <= targetUser.privilege)
        return "-1"; // 当前用户权限不足
    bool is_password_changed = password_ != "";
    bool is_name_changed = name_ != "";
    bool is_mailAddr_changed = mailAddr_ != "";
    bool is_privilege_changed = privilegeLevel_ != -1;
    bool is_password_valid = true, is_name_valid = true, is_privilege_valid = true;
    if (is_password_changed)
        is_password_valid = Validator::validate_password(password_);
    if (is_name_changed)
        is_name_valid = Validator::validate_name(name_);
    if (is_privilege_changed)
        is_privilege_valid = (privilegeLevel_ >= 0 && privilegeLevel_ <= 10);
    if (!is_password_valid || !is_name_valid || !is_privilege_valid)
        return "-1"; // 参数非法
    if (is_password_changed)
        targetUser.password = User::hash_password(password_, targetUser.salt);
    if (is_name_changed)
        std::strncpy(targetUser.name, name_.c_str(), sizeof(targetUser.name));
    if (is_mailAddr_changed)
        std::strncpy(targetUser.mailAddr, mailAddr_.c_str(), sizeof(targetUser.mailAddr));
    if (is_privilege_changed)
        targetUser.privilege = privilegeLevel_;

    // 写回数据库
    userDatabase.update(targetUser, target_index);

    return targetUser.to_string();
}

void UserManager::exit()
{
    // 所有在线用户下线
    logset.clear();
}

void UserManager::clean()
{
    // 清空所有用户数据与索引，并下线所有用户
    userIndex.clear();
    userDatabase.clear();
    logset.clear();
}
