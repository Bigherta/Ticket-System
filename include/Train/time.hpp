#ifndef TIME_HPP
#define TIME_HPP
#include <string>
struct Time
{
    int hour;
    int minute;
    Time() : hour(0), minute(0) {}
    Time(int h, int m) : hour(h), minute(m) {}
    Time(std::string time_str)
    {
        hour = std::stoi(time_str.substr(0, 2));
        minute = std::stoi(time_str.substr(3, 2));
    }
    int operator-(const Time &other) const { return (hour - other.hour) * 60 + (minute - other.minute); }
};

struct Date
{
    int month;
    int day;
    Date() : month(0), day(0) {}
    Date(int m, int d) : month(m), day(d) {}
    Date(std::string date_str)
    {
        month = std::stoi(date_str.substr(0, 2));
        day = std::stoi(date_str.substr(3, 2));
    }
    bool operator<(const Date &other) const
    {
        if (month != other.month)
            return month < other.month;
        return day < other.day;
    }
    bool operator==(const Date &other) const { return month == other.month && day == other.day; }
    bool operator<=(const Date &other) const { return *this < other || *this == other; }
};

struct AccurateTime
{
    Date date;
    Time time;
    AccurateTime() = default;
    AccurateTime(const Date &d, const Time &t) : date(d), time(t) {}
    AccurateTime operator+(int minutes) const
    {
        int total_minutes = time.hour * 60 + time.minute + minutes;
        int new_hour = (total_minutes / 60) % 24;
        int new_minute = total_minutes % 60;
        if (total_minutes / 60 >= 24)
        {
            Date new_date = date;
            new_date.day += (total_minutes / 60) / 24;
            if (new_date.month == 6 && new_date.day > 30)
            {
                new_date.month += new_date.day / 30;
                new_date.day = new_date.day % 30;
            }
            else if (new_date.month == 7 && new_date.day > 31)
            {
                new_date.month += new_date.day / 31;
                new_date.day = new_date.day % 31;
            }
            else if (new_date.month == 8 && new_date.day > 31)
            {
                new_date.month += new_date.day / 31;
                new_date.day = new_date.day % 31;
            }
            return AccurateTime(new_date, Time(new_hour, new_minute));
        }
        return AccurateTime(date, Time(new_hour, new_minute));
    }
    operator std::string() const
    {
        char buffer[20];
        std::snprintf(buffer, sizeof(buffer), "%02d-%02d %02d:%02d", date.month, date.day, time.hour, time.minute);
        return std::string(buffer);
    }
};
#endif // TIME_HPP
