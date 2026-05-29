#ifndef ORDER_HPP
#define ORDER_HPP
#include <string>
#include "../BPlusTree/BPT.hpp"
#include "../Grammar/Token.hpp"
#include "../Library/string_key.hpp"
#include "../Train/time.hpp"
#include "../Train/train.hpp"
#include "../User/user.hpp"
enum class orderState
{
    SUCCESS = 0,
    REFUNDED = 1,
    PENDING = 2
};
class OrderManager;
class Order
{
    friend class OrderManager;

private:
    int timestamp;
    char username[21]{};
    char train_id[21]{};
    char from[41]{};
    char to[41]{};
    AccurateTime depart_time;
    AccurateTime arrive_time;
    long long price;
    int num;
    orderState state;
    operator std::string() const
    {
        char buf[256];
        const char *state_str = (state == orderState::SUCCESS) ? "[success] " :
                                (state == orderState::REFUNDED) ? "[refunded] " :
                                                                  "[pending] ";
        std::snprintf(buf, sizeof(buf), "%s%s %s %02d-%02d %02d:%02d -> %s %02d-%02d %02d:%02d %lld %d",
                      state_str,
                      train_id,
                      from,
                      depart_time.date.month, depart_time.date.day,
                      depart_time.time.hour, depart_time.time.minute,
                      to,
                      arrive_time.date.month, arrive_time.date.day,
                      arrive_time.time.hour, arrive_time.time.minute,
                      price,
                      num);
        return std::string(buf);
    }

public:
    bool operator<(const Order &other) const { return timestamp < other.timestamp; }
    bool operator==(const Order &other) const { return timestamp == other.timestamp; }
};

class OrderManager
{
private:
    struct OrderInfo
    {
        int order_index;
        int timestamp;
        bool operator<(const OrderInfo &other) const { return timestamp < other.timestamp; }
        bool operator==(const OrderInfo &other) const { return timestamp == other.timestamp; }
    };
    BPT<sjtu::StringKey<21>, OrderInfo> user_order_map;
    BPT<int, int> waiting_orders; // orders in the queue, sorted by timestamp
    UserManager &user_manager;
    int accumulated_time = 0;
    MemoryRiver<Order> order_info_memory_river;
    BufferPoolManager<Order> *order_buffer_pool;

public:
    OrderManager(UserManager &user_manager) :
        user_order_map("user_order_bpt", 130), waiting_orders("waiting_orders", 35), user_manager(user_manager)
    {
        order_info_memory_river.get_info(accumulated_time, 1);
        order_info_memory_river.initialise("order_info_memory_river");
        order_buffer_pool = new BufferPoolManager<Order>(80, order_info_memory_river);
    }
    ~OrderManager() { delete order_buffer_pool; }
    /**
     * @brief Buy a ticket for a user
     * @param timestamp The timestamp of the order
     * @param username The username of the user
     * @param train_id The ID of the train
     * @param date The date of the journey
     * @param from The starting station
     * @param to The destination station
     * @param num The number of tickets to buy
     * @param added_into_queue Whether the order is added into the queue
     */
    std::string BuyTicket(int timestamp, TrainManager &train_manager, const std::string &username,
                          const std::string &train_id, const std::string &date, const std::string &from,
                          const std::string &to, int num, bool added_into_queue);

    /**
     * @brief Query orders for a user
     * @param username The username of the user
     */
    std::string queryOrder(const std::string &username) const;

    /**
     * @brief Refund a ticket for a user
     * @param username The username of the user
     * @param order_id The ID of the order to refund
     */
    std::string refundTicket(TrainManager &train_manager, const std::string &username, int order_id);

    /**
     * @brief Handle order-related commands
     * @param tokens The token stream containing the command and its parameters
     * @param train_manager The train manager instance
     * @return The result of the command execution
     */
    std::string handleOrderCommand(TokenStream &tokens, TrainManager &train_manager, int time_stamp);
    void clean()
    {
        user_order_map.clear();
        waiting_orders.clear();
        order_info_memory_river.clear();
    }

    void update_accumulated_time(int timestamp)
    {
        accumulated_time += timestamp;
        order_info_memory_river.write_info(accumulated_time, 1);
    }
};
#endif // ORDER_HPP
