#include "../include/Order/order.hpp"
#include <string>
std::string OrderManager::BuyTicket(int timestamp, TrainManager &train_manager, const std::string &username,
                                    const std::string &train_id, const std::string &date, const std::string &from,
                                    const std::string &to, int num, bool added_into_queue)
{
    char train_id_c[21]{};
    std::sprintf(train_id_c, "%s", train_id.c_str());
    auto train_index_vec = train_manager.trainIndex.visit(train_id_c);
    if (train_index_vec.empty())
    {
        return "-1";
    }
    auto train_index = train_index_vec[0];
    auto train = train_manager.trainBufferPool->get(train_index);
    // validate train release status
    if (!train.is_released)
        return "-1";
    // validate requested number
    if (num <= 0)
    {
        return "-1"; // cannot buy non-positive number of tickets
    }
    if (num > train.total_seat_num)
    {
        return "-1"; // cannot queue or buy more than the train's total seat number
    }
    int from_index = -1, to_index = -1;
    for (int i = 0; i < train.station_num; ++i)
    {
        if (std::strcmp(train.stations[i], from.c_str()) == 0)
            from_index = i;
        if (std::strcmp(train.stations[i], to.c_str()) == 0)
            to_index = i;
    }
    if (from_index == -1 || to_index == -1 || from_index >= to_index)
        return "-1";
    AccurateTime earliest = AccurateTime(train.start_date, train.start_time) + train.depart_time_offset[from_index];
    AccurateTime latest = AccurateTime(train.end_date, train.start_time) + train.depart_time_offset[from_index];
    Date query_d(date);
    if (query_d < earliest.date || latest.date < query_d)
        return "-1";
    int depart_minutes =
            (train.start_time.hour * 60 + train.start_time.minute + train.depart_time_offset[from_index]) % 1440;
    Time depart_clock(depart_minutes / 60, depart_minutes % 60);
    AccurateTime from_depart(Date(date), depart_clock);
    int travel_minutes = train.arrive_time_offset[to_index] - train.depart_time_offset[from_index];

    int min_seat = train.total_seat_num;
    for (int i = from_index; i < to_index; ++i)
    {
        AccurateTime segment_depart =
                from_depart + (train.depart_time_offset[i] - train.depart_time_offset[from_index]);
        TrainManager::SeatStatus key{};
        key.train_addr = train_index;
        std::sprintf(key.station, "%s", train.stations[i]);
        key.date = segment_depart.date;
        auto seat_list = train_manager.seat_manager.visit(key);
        int seat_left = seat_list.empty() ? train.total_seat_num : seat_list[0];
        if (seat_left < min_seat)
            min_seat = seat_left;
    }
    if (min_seat < num)
    {
        if (!added_into_queue)
        {
            return "-1";
        }
        else
        {
            Order order{};
            order.timestamp = timestamp;
            std::sprintf(order.username, "%s", username.c_str());
            std::sprintf(order.train_id, "%s", train_id.c_str());
            std::sprintf(order.from, "%s", from.c_str());
            std::sprintf(order.to, "%s", to.c_str());
            order.depart_time = from_depart;
            int arrive_minutes = (depart_minutes + travel_minutes) % 1440;
            Time arrive_clock(arrive_minutes / 60, arrive_minutes % 60);
            order.arrive_time = from_depart + travel_minutes;
            int start_price = (from_index == 0) ? 0 : train.prices_prefix[from_index];
            order.price = train.prices_prefix[to_index] - start_price;
            order.num = num;
            order.state = orderState::PENDING;
            char username_c[21]{};
            std::sprintf(username_c, "%s", username.c_str());
            user_order_map.insert(username_c, order);
            waiting_orders.emplace(order);
            return "queue";
        }
    }

    for (int i = from_index; i < to_index; ++i)
    {
        AccurateTime segment_depart =
                from_depart + (train.depart_time_offset[i] - train.depart_time_offset[from_index]);
        TrainManager::SeatStatus key{};
        key.train_addr = train_index;
        std::sprintf(key.station, "%s", train.stations[i]);
        key.date = segment_depart.date;
        auto seat_list = train_manager.seat_manager.visit(key);
        if (seat_list.empty())
            continue;
        int seat_left = seat_list[0] - num;
        train_manager.seat_manager.remove(key, seat_list[0]);
        train_manager.seat_manager.insert(key, seat_left);
    }
    Order order{};
    order.timestamp = timestamp;
    std::sprintf(order.username, "%s", username.c_str());
    std::sprintf(order.train_id, "%s", train_id.c_str());
    std::sprintf(order.from, "%s", from.c_str());
    std::sprintf(order.to, "%s", to.c_str());
    order.depart_time = from_depart;
    int arrive_minutes = (depart_minutes + travel_minutes) % 1440;
    Time arrive_clock(arrive_minutes / 60, arrive_minutes % 60);
    order.arrive_time = from_depart + travel_minutes;
    int start_price = (from_index == 0) ? 0 : train.prices_prefix[from_index];
    order.price = train.prices_prefix[to_index] - start_price;
    order.num = num;
    order.state = orderState::SUCCESS;
    char username_c[21]{};
    std::sprintf(username_c, "%s", username.c_str());
    user_order_map.insert(username_c, order);
    return std::to_string((long long) order.price * (long long) num);
}

std::string OrderManager::queryOrder(const std::string &username) const
{
    if (user_manager.logset.find(username) == user_manager.logset.end())
    {
        return "-1";
    }
    char username_c[21]{};
    std::sprintf(username_c, "%s", username.c_str());
    auto order_list = user_order_map.visit(username_c);
    if (order_list.empty())
        return "0";
    std::string res;
    res += std::to_string(order_list.size());
    res += "\n";
    for (int i = (int) order_list.size() - 1; i >= 0; --i)
    {
        res += std::string(order_list[i]);
        if (i != 0)
            res += "\n";
    }
    return res;
}

std::string OrderManager::refundTicket(TrainManager &train_manager, const std::string &username, int order_id = 1)
{
    if (user_manager.logset.find(username) == user_manager.logset.end())
    {
        return "-1";
    }
    char username_c[21]{};
    std::sprintf(username_c, "%s", username.c_str());
    auto order_list = user_order_map.visit(username_c);
    int order_index = (int) order_list.size() - order_id;
    if (order_index < 0 || order_index >= (int) order_list.size())
    {
        return "-1";
    }
    auto order = order_list[order_index];
    if (order.state == orderState::REFUNDED)
    {
        return "-1";
    }
    if (order.state == orderState::PENDING)
    {
        user_order_map.remove(username_c, order);
        order.state = orderState::REFUNDED;
        user_order_map.insert(username_c, order);
        waiting_orders.erase(order);
        return "0";
    }
    auto start_place = order.from;
    auto end_place = order.to;
    int number = order.num;
    char train_id[21]{};
    std::sprintf(train_id, "%s", order.train_id);
    auto train_index = train_manager.trainIndex.visit(train_id)[0];
    auto train = train_manager.trainBufferPool->get(train_index);
    int from_index = -1, to_index = -1;
    for (int i = 0; i < train.station_num; ++i)
    {
        if (std::strcmp(train.stations[i], start_place) == 0)
            from_index = i;
        if (std::strcmp(train.stations[i], end_place) == 0)
            to_index = i;
    }

    // Restore seats for the refunded order
    for (int i = from_index; i < to_index; ++i)
    {
        AccurateTime segment_depart =
                order.depart_time + (train.depart_time_offset[i] - train.depart_time_offset[from_index]);
        TrainManager::SeatStatus key{};
        key.train_addr = train_index;
        std::sprintf(key.station, "%s", train.stations[i]);
        key.date = segment_depart.date;
        auto seat_list = train_manager.seat_manager.visit(key);
        int seat_left = seat_list.empty() ? train.total_seat_num : seat_list[0];
        train_manager.seat_manager.remove(key, seat_left);
        train_manager.seat_manager.insert(key, seat_left + number);
    }

    user_order_map.remove(username_c, order);
    order.state = orderState::REFUNDED;
    user_order_map.insert(username_c, order);

    // Check waiting orders
    sjtu::vector<Order> fulfilled_orders;
    for (const auto &w_order: waiting_orders)
    {
        if (std::strcmp(w_order.train_id, train_id) != 0)
            continue;

        int w_from_index = -1, w_to_index = -1;
        for (int i = 0; i < train.station_num; ++i)
        {
            if (std::strcmp(train.stations[i], w_order.from) == 0)
                w_from_index = i;
            if (std::strcmp(train.stations[i], w_order.to) == 0)
                w_to_index = i;
        }

        // 1. Check if they belong to the exact same train instance (same starting day)
        // Since AccurateTime doesn't reliably support negative minute addition (subtraction),
        // we use cross-addition: depart1 + offset2 == depart2 + offset1
        if (order.depart_time + train.depart_time_offset[w_from_index] !=
            w_order.depart_time + train.depart_time_offset[from_index])
            continue;

        // 2. Check if the segments overlap. If not, this refund hasn't freed seats for this waiting order.
        if (std::max(from_index, w_from_index) >= std::min(to_index, w_to_index))
            continue;

        int min_seat = train.total_seat_num;
        for (int i = w_from_index; i < w_to_index; ++i)
        {
            AccurateTime segment_depart =
                    w_order.depart_time + (train.depart_time_offset[i] - train.depart_time_offset[w_from_index]);
            TrainManager::SeatStatus key{};
            key.train_addr = train_index;
            std::sprintf(key.station, "%s", train.stations[i]);
            key.date = segment_depart.date;
            auto seat_list = train_manager.seat_manager.visit(key);
            int seat_left = seat_list.empty() ? train.total_seat_num : seat_list[0];
            if (seat_left < min_seat)
                min_seat = seat_left;
        }

        if (min_seat >= w_order.num)
        {
            // Fulfill this waiting order!
            for (int i = w_from_index; i < w_to_index; ++i)
            {
                AccurateTime segment_depart =
                        w_order.depart_time + (train.depart_time_offset[i] - train.depart_time_offset[w_from_index]);
                TrainManager::SeatStatus key{};
                key.train_addr = train_index;
                std::sprintf(key.station, "%s", train.stations[i]);
                key.date = segment_depart.date;
                auto seat_list = train_manager.seat_manager.visit(key);
                int seat_left = seat_list.empty() ? train.total_seat_num : seat_list[0];
                train_manager.seat_manager.remove(key, seat_left);
                train_manager.seat_manager.insert(key, seat_left - w_order.num);
            }
            fulfilled_orders.push_back(w_order);
        }
    }

    for (size_t i = 0; i < fulfilled_orders.size(); ++i)
    {
        Order w_order = fulfilled_orders[i];
        waiting_orders.erase(w_order);
        user_order_map.remove(w_order.username, w_order);
        w_order.state = orderState::SUCCESS;
        user_order_map.insert(w_order.username, w_order);
    }

    return "0";
}

std::string OrderManager::handleOrderCommand(TokenStream &tokens, TrainManager &train_manager, int time_stamp)
{
    std::string username;
    std::string train_id;
    std::string query_date;
    std::string from;
    int num = 0;
    std::string to;
    std::string in_queue;

    const Token *cmd_token = tokens.get();
    if (cmd_token == nullptr)
        return "-1";

    switch (cmd_token->type)
    {
        case BUYTICKET: {
            if (tokens.size() < 7 || tokens.size() > 8)
                return "-1"; // 参数不足或过多
            in_queue = "false"; // 默认不加入候补队列
            const Token *param_token = tokens.get();
            while (param_token != nullptr)
            {
                switch (param_token->type)
                {
                    case USERNAME:
                        username = param_token->text;
                        break;
                    case TRAINID:
                        train_id = param_token->text;
                        break;
                    case QUERYDATE:
                        query_date = param_token->text;
                        break;
                    case STARTPLACE:
                        from = param_token->text;
                        break;
                    case DESTINATION:
                        to = param_token->text;
                        break;
                    case NUMBER:
                        num = std::stoi(param_token->text);
                        break;
                    case INQUEUE:
                        in_queue = param_token->text;
                        break;
                    default:
                        return "-1"; // 无效参数
                }
                param_token = tokens.get();
            }
            bool added_into_queue = (in_queue == "true");
            if (num <= 0)
                return "-1"; // cannot buy non-positive number of tickets
            return BuyTicket(time_stamp, train_manager, username, train_id, query_date, from, to, num,
                             added_into_queue);
        }
        case QUERYORDER: {
            if (tokens.size() != 2)
                return "-1";
            const Token *param_token = tokens.get();
            if (param_token == nullptr || param_token->type != USERNAME)
                return "-1";
            username = param_token->text;
            return queryOrder(username);
        }
        case REFUNDTICKET: {
            if (tokens.size() < 2 || tokens.size() > 3)
                return "-1";
            num = 1; // 默认退最近的一笔订单
            const Token *param_token = tokens.get();
            while (param_token != nullptr)
            {
                if (param_token->type == USERNAME)
                    username = param_token->text;
                else if (param_token->type == NUMBER)
                    num = std::stoi(param_token->text);
                else
                    return "-1";
                param_token = tokens.get();
            }
            return refundTicket(train_manager, username, num);
        }
        default:
            return "-1"; // 无效指令
    }
}
