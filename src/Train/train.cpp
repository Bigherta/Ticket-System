#include "../../include/Train/train.hpp"
#include <cstring>
#include <string>
#include "../../include/Validator/validator.hpp"

TrainManager::TrainManager() : trainIndex("trainIndex.dat") { trainDatabase.initialise("trainDatabase.dat"); }

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

    if (trainIndex.find(train_id))
        return "-1"; // train already exists

    Train newTrain;
    if (!Validator::validate_time(start_time, newTrain.start_time))
        return "-1"; // Invalid start time format
    if (!Validator::validate_saledate(sale_date, newTrain.start_date, newTrain.end_date))
        return "-1"; // Invalid sale date format
    std::strncpy(newTrain.train_id, train_id.c_str(), 20);
    newTrain.train_id[20] = '\0';
    newTrain.station_num = std::stoi(station_num);
    newTrain.seat_num = std::stoi(seat_num);
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

    int addr = trainDatabase.write(newTrain);
    trainIndex.insert(train_id, addr);
    return "0";
}

std::string TrainManager::deleteTrain(const std::string &train_id)
{
    if (!Validator::validate_trainid(train_id) || releasedTrains.find(train_id) != releasedTrains.end())
        return "-1"; // Invalid train ID
    auto pos_list = trainIndex.visit(train_id);
    if (pos_list.empty())
        return "-1"; // train does not exist
    Train temp;
    trainDatabase.read(temp, pos_list[0]);
    if (temp.is_released)
    {
        releasedTrains.emplace(train_id);
        return "-1"; // train has been released, cannot be deleted
    }
    trainIndex.remove(train_id, pos_list[0]);
    trainDatabase.Delete(pos_list[0]);
    return "0";
}

std::string TrainManager::releaseTrain(const std::string &train_id)
{
    if (!Validator::validate_trainid(train_id) || releasedTrains.find(train_id) != releasedTrains.end())
        return "-1"; // Invalid train ID
    auto pos_list = trainIndex.visit(train_id);
    if (pos_list.empty())
        return "-1"; // train does not exist
    Train temp;
    trainDatabase.read(temp, pos_list[0]);
    if (temp.is_released)
    {
        releasedTrains.emplace(train_id);
        return "-1"; // train has been released, cannot be released again
    }
    temp.is_released = true;
    trainDatabase.update(temp, pos_list[0]);
    releasedTrains.emplace(train_id);
    return "0";
}

std::string TrainManager::queryTrain(const std::string &train_id, const std::string &date)
{
    // TODO: Implement queryTrain
    if (!Validator::validate_trainid(train_id))
        return "-1"; // Invalid parameters

    auto pos_list = trainIndex.visit(train_id);
    if (pos_list.empty())
        return "-1"; // train does not exist

    Train targetTrain;
    trainDatabase.read(targetTrain, pos_list[0]);
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
        if (i == targetTrain.station_num - 1)
        {
            result += "xx-xx xx:xx ";
        }
        else
        {
            AccurateTime depart_time = startTime + targetTrain.depart_time_offset[i];
            result += std::string(depart_time) + " ";
        }
        result += std::to_string(targetTrain.prices_prefix[i]) + " ";
        if (i == targetTrain.station_num - 1)
            result += "x";
        else
            result += std::to_string(targetTrain.seat_num) + "\n";
    }
    return result;
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
            return addTrain(train_id, station_num, seat_num, stations, prices, start_time, travel_time,
                            stopover_time, sale_date, type);
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
        default:
            return "-1";
    }
}

void TrainManager::clean()
{
    // TODO: Implement data cleanup
    trainIndex.clear();
    trainDatabase.clear();
    releasedTrains.clear();
}
