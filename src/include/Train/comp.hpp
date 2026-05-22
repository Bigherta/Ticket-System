#ifndef COMP_HPP
#define COMP_HPP
#include <cstring>
#include "time.hpp"
struct TrainRoute
{
	char train_id[21]{};
	char from[41]{};
	char to[41]{};
	AccurateTime depart_time;
	AccurateTime arrive_time;
	int total_time = 0;
	int total_price = 0;
	int seat = 0;
};

struct TrainTransferRouteTime
{
    char first_train_id[21]{};
    char second_train_id[21]{};
    char from[41]{};
    char transfer_station[41]{};
    char to[41]{};
    AccurateTime first_depart_time;
    AccurateTime first_arrive_time;
    AccurateTime second_depart_time;
    AccurateTime second_arrive_time;
    int price_first_train = 0;
    int price_second_train = 0;
    int total_time = 0;
    int total_price = 0;
    int seat_first_train = 0;
    int seat_second_train = 0;
    bool operator<(const TrainTransferRouteTime &other) const
    {
        if (total_time != other.total_time)
            return total_time < other.total_time;
        if (total_price != other.total_price)
            return total_price < other.total_price;
        if (std::strcmp(first_train_id, other.first_train_id) != 0)
            return std::strcmp(first_train_id, other.first_train_id) < 0;
        return std::strcmp(second_train_id, other.second_train_id) < 0;
    }    
};

struct TrainTransferRoutePrice
{
    char first_train_id[21]{};
    char second_train_id[21]{};
    char from[41]{};
    char transfer_station[41]{};
    char to[41]{};
    AccurateTime first_depart_time;
    AccurateTime first_arrive_time;
    AccurateTime second_depart_time;
    AccurateTime second_arrive_time;
    int price_first_train = 0;
    int price_second_train = 0;
    int total_time = 0;
    int total_price = 0;
    int seat_first_train = 0;
    int seat_second_train = 0;
    bool operator<(const TrainTransferRoutePrice &other) const
    {
        if (total_price != other.total_price)
            return total_price < other.total_price;
        if (total_time != other.total_time)
            return total_time < other.total_time;
        if (std::strcmp(first_train_id, other.first_train_id) != 0)
            return std::strcmp(first_train_id, other.first_train_id) < 0;
        return std::strcmp(second_train_id, other.second_train_id) < 0;
    }
};
struct TrainTimeGreater
{
	bool operator()(const TrainRoute &lhs, const TrainRoute &rhs) const
	{
		if (lhs.total_time != rhs.total_time)
			return lhs.total_time > rhs.total_time;
		return std::strcmp(lhs.train_id, rhs.train_id) > 0;
	}
};

struct TrainPriceGreater
{
	bool operator()(const TrainRoute &lhs, const TrainRoute &rhs) const
	{
		if (lhs.total_price != rhs.total_price)
			return lhs.total_price > rhs.total_price;
		return std::strcmp(lhs.train_id, rhs.train_id) > 0;
	}
};

#endif