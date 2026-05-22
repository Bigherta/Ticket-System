#ifndef ORDER_HPP
#define ORDER_HPP
#include <string>
#include "../Train/train.hpp"
#include "../Train/time.hpp"
#include "../BPlusTree/BPT.hpp"
enum class orderState
{
    SUCCESS = 0,
    REFUNDED = 1,
    PENDING = 2
};
class Order
{
private:
    char train_id[21]{};
    char from[41]{};
    char to[41]{};
    AccurateTime depart_time;
    AccurateTime arrive_time;
    int price;
    int num;
    orderState state;
    operator std::string() const
    {
        std::string res;
        if (state == orderState::SUCCESS)
            res += "[success] ";
        else if (state == orderState::REFUNDED)
            res += "[refunded] ";
        else
            res += "[pending] ";
        res += train_id;
        res += " ";
        res += from;
        res += " ";
        res += std::string(depart_time);
        res += " -> ";
        res += to;
        res += " ";
        res += std::string(arrive_time);
        res += " ";
        res += std::to_string(price);
        res += " ";
        res += std::to_string(num);
        return res;
    }
};

class OrderManager
{
    private : 
    BPT<char [21], Order> order_tree;
    public:
    /**
     * @brief Buy a ticket for a user
     * @param username The username of the user
     * @param train_id The ID of the train
     * @param date The date of the journey
     * @param from The starting station
     * @param to The destination station
     * @param num The number of tickets to buy
     * @param added_into_queue Whether the order is added into the queue
     */
    std::string BuyTicket(TrainManager &train_manager, const std::string & username, const std::string &train_id,const std::string & date,  const std::string &from, const std::string &to, int num, bool added_into_queue);

    /**
     * @brief Query orders for a user
     * @param username The username of the user
     */
    std::string queryOrder(const std::string & username) const;

    /**
     * @brief Refund a ticket for a user
     * @param username The username of the user
     * @param order_id The ID of the order to refund
     */
    std::string refundTicket(TrainManager &train_manager, const std::string & username, int order_id);
};
#endif // ORDER_HPP
