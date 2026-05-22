#include "../include/Order/order.hpp"
std::string OrderManager::BuyTicket(TrainManager &train_manager, const std::string &username, const std::string &train_id, const std::string &date,
                             const std::string &from, const std::string &to, int num, bool added_into_queue)
{
    char train_id_c[21]{};
    std::sprintf(train_id_c, "%s", train_id.c_str());
    auto train_index  = train_manager.trainIndex.visit(train_id_c)[0];
    auto train = train_manager.trainBufferPool->get(train_index);
    int from_index = -1, to_index = -1;
    for (int i = 0; i < train.station_num; ++i)
    {
        if (std::strcmp(train.stations[i], from.c_str()) == 0)
            from_index = i;
        if (std::strcmp(train.stations[i], to.c_str()) == 0)
            to_index = i;
    }
    int depart_minutes = (train.start_time.hour * 60 + train.start_time.minute +
                          train.depart_time_offset[from_index]) % 1440;
    Time depart_clock(depart_minutes / 60, depart_minutes % 60);
    AccurateTime from_depart(Date(date), depart_clock);

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
        if (seat_list.empty())
        {
            min_seat = 0;
            break;
        }
        if (seat_list[0] < min_seat)
            min_seat = seat_list[0];
    }
    if (min_seat < num)
        return added_into_queue ? "queue" : "-1";

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
    return std::to_string(train.prices_prefix[to_index] - train.prices_prefix[from_index - 1]);
}

std::string OrderManager::queryOrder(const std::string &username) const {}

std::string OrderManager::refundTicket(TrainManager &train_manager, const std::string &username, int order_id) {}
