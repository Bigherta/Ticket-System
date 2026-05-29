#include "../include/Train/train.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include "../include/Library/priority_queue.hpp"
#include "../include/Train/comp.hpp"
#include "../include/Validator/validator.hpp"

TrainManager::TrainManager() :
    trainIndex("trainIndex.dat", 200), station_train_mapping("stationTrainMapping.dat", 200),
    trainSegmentIndex("trainSegmentIndex.dat", 500), seat_manager("seatManager.dat", 350),
    trainBufferPool(new BufferPoolManager<Train>(300, trainDatabase))
{
    trainDatabase.initialise("trainDatabase.dat");
}

TrainManager::~TrainManager() { delete trainBufferPool; }

std::string TrainManager::addTrain(const std::string &train_id, const std::string &station_num,
                                   const std::string &seat_num, const std::string &stations, const std::string &prices,
                                   const std::string &start_time, const std::string &travel_time,
                                   const std::string &stopover_time, const std::string &sale_date,
                                   const std::string &type)
{
    auto split = [](const std::string &input, char delim) {
        sjtu::vector<std::string> result;
        size_t start = 0;
        while (true)
        {
            size_t pos = input.find(delim, start);
            if (pos == std::string::npos)
            {
                result.push_back(input.substr(start));
                break;
            }
            result.push_back(input.substr(start, pos - start));
            start = pos + 1;
        }
        return result;
    };

    if (!Validator::validate_trainid(train_id) || !Validator::validate_type(type) ||
        !Validator::validate_number(station_num, TokenType::STATIONNUM) ||
        !Validator::validate_number(seat_num, TokenType::SEATNUM))
        return "-1"; // Invalid parameters or train already exists

    char train_id_key[21]{};
    std::strncpy(train_id_key, train_id.c_str(), 20);
    train_id_key[20] = '\0';

    if (trainIndex.find(train_id_key))
        return "-1"; // train already exists

    Train newTrain;
    if (!Validator::validate_time(start_time, newTrain.start_time))
        return "-1"; // Invalid start time format
    if (!Validator::validate_saledate(sale_date, newTrain.start_date, newTrain.end_date))
        return "-1"; // Invalid sale date format
    std::strncpy(newTrain.train_id, train_id_key, 20);
    newTrain.train_id[20] = '\0';
    newTrain.station_num = std::stoi(station_num);
    newTrain.total_seat_num = std::stoi(seat_num);
    newTrain.type = type[0];
    sjtu::vector<std::string> station_list = split(stations, '|');
    sjtu::vector<std::string> price_list = split(prices, '|');
    sjtu::vector<std::string> travel_list = split(travel_time, '|');
    sjtu::vector<std::string> stopover_list;
    if (newTrain.station_num > 2)
        stopover_list = split(stopover_time, '|');
    newTrain.addStation(station_list);
    newTrain.addPrices(price_list);
    newTrain.addTimeoffset(travel_list, stopover_list);

    int addr = trainBufferPool->new_page(newTrain);
    trainIndex.insert(train_id_key, addr);
    return "0";
}

std::string TrainManager::deleteTrain(const std::string &train_id)
{
    if (!Validator::validate_trainid(train_id) || releasedTrains.find(train_id) != releasedTrains.end())
        return "-1"; // Invalid train ID
    char train_id_key[21]{};
    std::strncpy(train_id_key, train_id.c_str(), 20);
    train_id_key[20] = '\0';
    auto pos_list = trainIndex.visit(train_id_key);
    if (pos_list.empty())
        return "-1"; // train does not exist
    Train temp = trainBufferPool->get(pos_list[0]);
    if (temp.is_released)
    {
        releasedTrains.emplace(train_id);
        return "-1"; // train has been released, cannot be deleted
    }
    trainIndex.remove(train_id_key, pos_list[0]);
    trainBufferPool->Delete(pos_list[0]);
    return "0";
}

std::string TrainManager::releaseTrain(const std::string &train_id)
{
    if (!Validator::validate_trainid(train_id) || releasedTrains.find(train_id) != releasedTrains.end())
        return "-1"; // Invalid train ID
    char train_id_key[21]{};
    std::strncpy(train_id_key, train_id.c_str(), 20);
    train_id_key[20] = '\0';
    auto pos_list = trainIndex.visit(train_id_key);
    if (pos_list.empty())
        return "-1"; // train does not exist
    Train temp = trainBufferPool->get(pos_list[0]);
    if (temp.is_released)
    {
        releasedTrains.emplace(train_id);
        return "-1"; // train has been released, cannot be released again
    }
    temp.is_released = true;
    trainBufferPool->put(pos_list[0], temp);
    releasedTrains.emplace(train_id);
    for (int i = 0; i < temp.station_num; ++i)
    {
        station_train_mapping.insert(temp.stations[i], {pos_list[0], i});
    }
    for (int i = 0; i < temp.station_num; ++i)
    {
        for (int j = i + 1; j < temp.station_num; ++j)
        {
            sjtu::pair<sjtu::StringKey<41>, sjtu::StringKey<41>> segment_key(
                sjtu::StringKey<41>(temp.stations[i]), sjtu::StringKey<41>(temp.stations[j]));
            TrainSegment seg{};
            seg.train_addr = pos_list[0];
            seg.start_index = i;
            seg.end_index = j;
            trainSegmentIndex.insert(segment_key, seg);
        }
    }
    auto next_date = [](const Date &current) {
        Date next = current;
        ++next.day;
        if (next.month == 6 && next.day > 30)
        {
            next.month = 7;
            next.day = 1;
        }
        else if (next.month == 7 && next.day > 31)
        {
            next.month = 8;
            next.day = 1;
        }
        else if (next.month == 8 && next.day > 31)
        {
            next.month = 9;
            next.day = 1;
        }
        return next;
    };

    for (Date run_date = temp.start_date; run_date <= temp.end_date; run_date = next_date(run_date))
    {
        AccurateTime start_time(run_date, temp.start_time);
        for (int i = 0; i < temp.station_num - 1; ++i)
        {
            AccurateTime depart_time = start_time + temp.depart_time_offset[i];
            SeatStatus key{};
            key.train_addr = pos_list[0];
            key.station_index = i;
            key.date = depart_time.date;
            seat_manager.insert(key, temp.total_seat_num);
        }
    }
    return "0";
}

std::string TrainManager::queryTrain(const std::string &train_id, const std::string &date)
{
    // TODO: Implement queryTrain
    if (!Validator::validate_trainid(train_id))
        return "-1"; // Invalid parameters

    char train_id_key[21]{};
    std::strncpy(train_id_key, train_id.c_str(), 20);
    train_id_key[20] = '\0';
    auto pos_list = trainIndex.visit(train_id_key);
    if (pos_list.empty())
        return "-1"; // train does not exist

    Train targetTrain;
    targetTrain = trainBufferPool->get(pos_list[0]);
    auto start_date = targetTrain.start_date;
    auto end_date = targetTrain.end_date;
    Date query_date(date);
    if (query_date < start_date || end_date < query_date)
        return "-1"; // train does not run on the given date
    std::string result = train_id + " " + targetTrain.type + "\n";
    AccurateTime startTime(query_date, targetTrain.start_time);
    for (int i = 0; i < targetTrain.station_num; ++i)
    {
        result += std::string(targetTrain.stations[i]) + " ";
        if (i == 0)
        {
            result += "xx-xx xx:xx";
        }
        else
        {
            AccurateTime arrive_time = startTime + targetTrain.arrive_time_offset[i];
            result += std::string(arrive_time);
        }
        result += " -> ";
        AccurateTime depart_time;
        if (i == targetTrain.station_num - 1)
        {
            result += "xx-xx xx:xx ";
        }
        else
        {
            depart_time = startTime + targetTrain.depart_time_offset[i];
            result += std::string(depart_time) + " ";
        }
        result += std::to_string(targetTrain.prices_prefix[i]) + " ";
        if (i == targetTrain.station_num - 1)
        {
            result += "x";
        }
        else
        {
            int seat_left = targetTrain.total_seat_num;
            if (targetTrain.is_released)
            {
                SeatStatus key{};
                key.train_addr = pos_list[0];
                key.station_index = i;
                key.date = depart_time.date;
                auto seat_list = seat_manager.visit(key);
                if (!seat_list.empty())
                    seat_left = seat_list[0];
            }
            result += std::to_string(seat_left) + "\n";
        }
    }
    return result;
}

std::string TrainManager::queryTicket(const std::string &from, const std::string &to, const std::string &date,
                                      const std::string &priority = "time")
{
    sjtu::priority_queue<TrainRoute, TrainTimeGreater> *time_queue = nullptr;
    sjtu::priority_queue<TrainRoute, TrainPriceGreater> *price_queue = nullptr;
    if (priority == "time")
    {
        time_queue = new sjtu::priority_queue<TrainRoute, TrainTimeGreater>();
    }
    else if (priority == "cost")
    {
        price_queue = new sjtu::priority_queue<TrainRoute, TrainPriceGreater>();
    }
    else
    {
        return "-1"; // invalid priority
    }
    sjtu::pair<sjtu::StringKey<41>, sjtu::StringKey<41>> segment_key(
        sjtu::StringKey<41>(from.c_str()), sjtu::StringKey<41>(to.c_str()));
    auto segment_list = trainSegmentIndex.visit(segment_key);
    std::string result;
    Date query_date(date);
    for (const auto &seg: segment_list)
    {
        auto from_index = seg.start_index;
        auto to_index = seg.end_index;
        auto train = trainBufferPool->get(seg.train_addr);
        AccurateTime earliest_depart_time =
                AccurateTime(train.start_date, train.start_time) + train.depart_time_offset[from_index];
        AccurateTime latest_depart_time =
                AccurateTime(train.end_date, train.start_time) + train.depart_time_offset[from_index];
        if (query_date < earliest_depart_time.date || latest_depart_time.date < query_date)
            continue; // train does not run on the given date
        TrainRoute route;
        std::strncpy(route.train_id, train.train_id, 20);
        route.train_id[20] = '\0';
        std::strncpy(route.from, train.stations[from_index], 40);
        route.from[40] = '\0';
        std::strncpy(route.to, train.stations[to_index], 40);
        route.to[40] = '\0';
        int travel_minutes = train.arrive_time_offset[to_index] - train.depart_time_offset[from_index];
        int depart_minutes =
                (train.start_time.hour * 60 + train.start_time.minute + train.depart_time_offset[from_index]) %
                1440;
        Time depart_clock(depart_minutes / 60, depart_minutes % 60);
        route.depart_time = AccurateTime(query_date, depart_clock);
        route.arrive_time = route.depart_time + travel_minutes;
        route.total_time = travel_minutes;
        route.total_price = train.prices_prefix[to_index] - train.prices_prefix[from_index];
        int seat_left = train.total_seat_num;
        for (int i = from_index; i < to_index; ++i)
        {
            AccurateTime segment_depart =
                    route.depart_time + (train.depart_time_offset[i] - train.depart_time_offset[from_index]);
            SeatStatus key{};
            key.train_addr = seg.train_addr;
            key.station_index = i;
            key.date = segment_depart.date;
            auto seat_list = seat_manager.visit(key);
            if (!seat_list.empty() && seat_list[0] < seat_left)
                seat_left = seat_list[0];
        }
        route.seat = seat_left;
        if (priority == "time")
        {
            time_queue->push(route);
        }
        else if (priority == "cost")
        {
            price_queue->push(route);
        }
    }
    auto append_results = [&](auto *queue) {
        result += std::to_string(queue->size());
        if (!queue->empty())
            result += "\n";
        while (!queue->empty())
        {
            const TrainRoute &route = queue->top();
            result += std::string(route.train_id) + " ";
            result += std::string(route.from) + " ";
            result += std::string(route.depart_time) + " -> ";
            result += std::string(route.to) + " ";
            result += std::string(route.arrive_time) + " ";
            result += std::to_string(route.total_price) + " ";
            result += std::to_string(route.seat);
            queue->pop();
            if (!queue->empty())
                result += "\n";
        }
    };

    if (priority == "time")
    {
        append_results(time_queue);
    }
    else if (priority == "cost")
    {
        append_results(price_queue);
    }
    if (time_queue)
        delete time_queue;
    if (price_queue)
        delete price_queue;
    return result;
}

std::string TrainManager::queryTransfer(const std::string &from, const std::string &to, const std::string &date,
                                        const std::string &priority = "time")
{
    TrainTransferRouteTime *best_time_route = nullptr;
    TrainTransferRoutePrice *best_price_route = nullptr;
    if (priority == "time")
    {
        best_time_route = new TrainTransferRouteTime();
    }
    else if (priority == "cost")
    {
        best_price_route = new TrainTransferRoutePrice();
    }
    else
    {
        return "-1"; // invalid priority
    }
    char from_key[41]{};
    char to_key[41]{};
    std::sprintf(from_key, "%s", from.c_str());
    std::sprintf(to_key, "%s", to.c_str());
    auto from_list = station_train_mapping.visit(from_key);
    auto to_list = station_train_mapping.visit(to_key);
    if (from_list.empty() || to_list.empty())
        return "0"; // no train available
    Date query_date(date);
    auto min_seat_for_leg = [&](const Train &train, int train_addr, int from_idx, int to_idx,
                                const AccurateTime &from_depart) {
        int seat_left = train.total_seat_num;
        for (int i = from_idx; i < to_idx; ++i)
        {
            AccurateTime segment_depart =
                    from_depart + (train.depart_time_offset[i] - train.depart_time_offset[from_idx]);
            SeatStatus key{};
            key.train_addr = train_addr;
            key.station_index = i;
            key.date = segment_depart.date;
            auto seat_list = seat_manager.visit(key);
            if (!seat_list.empty() && seat_list[0] < seat_left)
                seat_left = seat_list[0];
        }
        return seat_left;
    };
    for (const auto &from_entry: from_list)
    {
        auto from_train = trainBufferPool->get(from_entry.first);
        AccurateTime earliest_depart_time = AccurateTime(from_train.start_date, from_train.start_time) +
                                            from_train.depart_time_offset[from_entry.second];
        AccurateTime latest_depart_time = AccurateTime(from_train.end_date, from_train.start_time) +
                                          from_train.depart_time_offset[from_entry.second];
        if (query_date < earliest_depart_time.date || latest_depart_time.date < query_date)
            continue; // train does not run on the given date
        int depart_minutes = (from_train.start_time.hour * 60 + from_train.start_time.minute +
                              from_train.depart_time_offset[from_entry.second]) %
                             1440;
        Time depart_clock(depart_minutes / 60, depart_minutes % 60);
        AccurateTime depart_time(query_date, depart_clock);
        sjtu::unordered_map<std::string, int> station_to_index;
        for (int i = from_entry.second + 1; i < from_train.station_num; ++i)
        {
            station_to_index[from_train.stations[i]] = i;
        }
        for (const auto &to_entry: to_list)
        {
            auto to_train = trainBufferPool->get(to_entry.first);
            for (int i = 0; i < to_entry.second; ++i)
            {
                if (to_entry.first != from_entry.first &&
                    station_to_index.find(to_train.stations[i]) != station_to_index.end())
                {
                    int transfer_index = station_to_index[to_train.stations[i]];
                    if (transfer_index <= from_entry.second)
                        continue; // transfer station must be after departure station in from_train
                    const char *from_station = from_train.stations[from_entry.second];
                    const char *seg_station = to_train.stations[i];
                    const char *to_station = to_train.stations[to_entry.second];
                    AccurateTime arrive_time = depart_time + (from_train.arrive_time_offset[transfer_index] -
                                                              from_train.depart_time_offset[from_entry.second]);
                    AccurateTime to_earliest_depart_time =
                            AccurateTime(to_train.start_date, to_train.start_time) + to_train.depart_time_offset[i];
                    AccurateTime to_latest_depart_time =
                            AccurateTime(to_train.end_date, to_train.start_time) + to_train.depart_time_offset[i];
                    AccurateTime next_depart_time(arrive_time.date, to_earliest_depart_time.time);
                    while (next_depart_time < arrive_time)
                        next_depart_time = next_depart_time + 1440;
                    if (next_depart_time < to_earliest_depart_time)
                        next_depart_time = to_earliest_depart_time;
                    if (to_latest_depart_time < next_depart_time)
                        continue; // cannot catch the transfer train

                    auto fill_transfer_route = [&](auto &route) {
                        std::strncpy(route.first_train_id, from_train.train_id, 20);
                        route.first_train_id[20] = '\0';
                        std::strncpy(route.second_train_id, to_train.train_id, 20);
                        route.second_train_id[20] = '\0';
                        std::strncpy(route.from, from_station, 40);
                        route.from[40] = '\0';
                        std::strncpy(route.transfer_station, seg_station, 40);
                        route.transfer_station[40] = '\0';
                        std::strncpy(route.to, to_station, 40);
                        route.to[40] = '\0';
                        route.first_depart_time = depart_time;
                        route.first_arrive_time = arrive_time;
                        route.second_depart_time = next_depart_time;
                        route.second_arrive_time =
                                route.second_depart_time +
                                (to_train.arrive_time_offset[to_entry.second] - to_train.depart_time_offset[i]);
                        route.total_time = route.second_arrive_time - route.first_depart_time;
                        route.price_first_train =
                                from_train.prices_prefix[transfer_index] - from_train.prices_prefix[from_entry.second];
                        route.price_second_train = to_train.prices_prefix[to_entry.second] - to_train.prices_prefix[i];
                        route.total_price = route.price_first_train + route.price_second_train;
                        route.seat_first_train = min_seat_for_leg(from_train, from_entry.first, from_entry.second,
                                                                  transfer_index, depart_time);
                        route.seat_second_train =
                                min_seat_for_leg(to_train, to_entry.first, i, to_entry.second, next_depart_time);
                    };

                    if (priority == "time")
                    {
                        TrainTransferRouteTime route;
                        fill_transfer_route(route);
                        if (best_time_route->total_time == 0 || route < *best_time_route)
                        {
                            *best_time_route = route;
                        }
                    }
                    else if (priority == "cost")
                    {
                        TrainTransferRoutePrice route;
                        fill_transfer_route(route);
                        if (best_price_route->total_price == 0 || route < *best_price_route)
                        {
                            *best_price_route = route;
                        }
                    }
                }
            }
        }
    }
    auto format_transfer_result = [](const auto &route) {
        std::string result;
        result += std::string(route.first_train_id) + " ";
        result += std::string(route.from) + " ";
        result += std::string(route.first_depart_time) + " -> ";
        result += std::string(route.transfer_station) + " ";
        result += std::string(route.first_arrive_time) + " ";
        result += std::to_string(route.price_first_train) + " ";
        result += std::to_string(route.seat_first_train) + "\n";
        result += std::string(route.second_train_id) + " ";
        result += std::string(route.transfer_station) + " ";
        result += std::string(route.second_depart_time) + " -> ";
        result += std::string(route.to) + " ";
        result += std::string(route.second_arrive_time) + " ";
        result += std::to_string(route.price_second_train) + " ";
        result += std::to_string(route.seat_second_train);
        return result;
    };

    if (priority == "time")
    {
        if (best_time_route->total_time == 0)
        {
            delete best_time_route;
            return "0"; // no transfer route available
        }
        std::string result = format_transfer_result(*best_time_route);
        delete best_time_route;
        return result;
    }
    else
    {
        if (best_price_route->total_price == 0)
        {
            delete best_price_route;
            return "0"; // no transfer route available
        }
        std::string result = format_transfer_result(*best_price_route);
        delete best_price_route;
        return result;
    }
}

std::string TrainManager::handleTrainCommand(TokenStream &tokens)
{
    std::string train_id;
    std::string station_num;
    std::string seat_num;
    std::string stations;
    std::string prices;
    std::string start_time;
    std::string travel_time;
    std::string stopover_time;
    std::string sale_date;
    std::string type;
    std::string query_date;
    std::string from;
    std::string to;
    std::string priority;

    const Token *cmd_token = tokens.get();
    if (cmd_token == nullptr)
        return "-1";

    switch (cmd_token->type)
    {
        case ADDTRAIN: {
            if (tokens.size() != 11)
                return "-1"; // 参数不足
            const Token *param_token = tokens.get();
            while (param_token != nullptr)
            {
                switch (param_token->type)
                {
                    case TRAINID:
                        train_id = param_token->text;
                        break;
                    case STATIONNUM:
                        station_num = param_token->text;
                        break;
                    case SEATNUM:
                        seat_num = param_token->text;
                        break;
                    case STATIONS:
                        stations = param_token->text;
                        break;
                    case PRICES:
                        prices = param_token->text;
                        break;
                    case STARTTIME:
                        start_time = param_token->text;
                        break;
                    case TRAVELTIMES:
                        travel_time = param_token->text;
                        break;
                    case STOPOVERTIMES:
                        stopover_time = param_token->text;
                        break;
                    case SALEDATE:
                        sale_date = param_token->text;
                        break;
                    case TRAINTYPE:
                        type = param_token->text;
                        break;
                    default:
                        return "-1"; // 参数类型错误
                }
                param_token = tokens.get();
            }
            return addTrain(train_id, station_num, seat_num, stations, prices, start_time, travel_time, stopover_time,
                            sale_date, type);
        }
        case DELETETRAIN: {
            if (tokens.size() != 2)
                return "-1";
            const Token *param_token = tokens.get();
            while (param_token != nullptr)
            {
                if (param_token->type == TRAINID)
                    train_id = param_token->text;
                else
                    return "-1";
                param_token = tokens.get();
            }
            return deleteTrain(train_id);
        }
        case RELEASETRAIN: {
            if (tokens.size() != 2)
                return "-1";
            const Token *param_token = tokens.get();
            while (param_token != nullptr)
            {
                if (param_token->type == TRAINID)
                    train_id = param_token->text;
                else
                    return "-1";
                param_token = tokens.get();
            }
            return releaseTrain(train_id);
        }
        case QUERYTRAIN: {
            if (tokens.size() != 3)
                return "-1";
            const Token *param_token = tokens.get();
            while (param_token != nullptr)
            {
                if (param_token->type == TRAINID)
                    train_id = param_token->text;
                else if (param_token->type == QUERYDATE)
                    query_date = param_token->text;
                else
                    return "-1";
                param_token = tokens.get();
            }
            return queryTrain(train_id, query_date);
        }
        case QUERYTICKET: {
            if (tokens.size() < 4 || tokens.size() > 5)
                return "-1";
            priority = "time"; // default priority
            const Token *param_token = tokens.get();
            while (param_token != nullptr)
            {
                if (param_token->type == STARTPLACE)
                    from = param_token->text;
                else if (param_token->type == DESTINATION)
                    to = param_token->text;
                else if (param_token->type == QUERYDATE)
                    query_date = param_token->text;
                else if (param_token->type == PRIORITY)
                    priority = param_token->text;
                else
                    return "-1";
                param_token = tokens.get();
            }
            return queryTicket(from, to, query_date, priority);
        }
        case QUERYTRANSFER: {
            if (tokens.size() < 4 || tokens.size() > 5)
                return "-1";
            priority = "time"; // default priority
            const Token *param_token = tokens.get();
            while (param_token != nullptr)
            {
                if (param_token->type == STARTPLACE)
                    from = param_token->text;
                else if (param_token->type == DESTINATION)
                    to = param_token->text;
                else if (param_token->type == QUERYDATE)
                    query_date = param_token->text;
                else if (param_token->type == PRIORITY)
                    priority = param_token->text;
                else
                    return "-1";
                param_token = tokens.get();
            }
            return queryTransfer(from, to, query_date, priority);
        }
        default:
            return "-1";
    }
}

void TrainManager::clean()
{
    // TODO: Implement data cleanup
    trainIndex.clear();
    station_train_mapping.clear();
    trainSegmentIndex.clear();
    seat_manager.clear();
    trainBufferPool->flush_all();
    trainDatabase.clear();
    releasedTrains.clear();
}
