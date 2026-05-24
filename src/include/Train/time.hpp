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
    bool operator<(const Time &other) const
    {
        if (hour != other.hour)
            return hour < other.hour;
        return minute < other.minute;
    }
    bool operator==(const Time &other) const { return hour == other.hour && minute == other.minute; }
    bool operator!=(const Time &other) const { return !(*this == other); }
    bool operator<=(const Time &other) const { return *this < other || *this == other; }
    bool operator>(const Time &other) const { return !(*this <= other); }
    bool operator>=(const Time &other) const { return !(*this < other); }
};

struct Date
{
    int month;
    int day;
    Date() : month(0), day(0) {}
    Date(int m, int d) : month(m), day(d) {}
    Date(const std::string &date_str)
    {
        month = std::stoi(date_str.substr(0, 2));
        day = std::stoi(date_str.substr(3, 2));
    }
    bool operator!=(const Date &other) const { return month != other.month || day != other.day; }
    bool operator<(const Date &other) const
    {
        if (month != other.month)
            return month < other.month;
        return day < other.day;
    }
    bool operator==(const Date &other) const { return month == other.month && day == other.day; }
    bool operator<=(const Date &other) const { return *this < other || *this == other; }
    bool operator>(const Date &other) const { return !(*this <= other); }
    bool operator>=(const Date &other) const { return !(*this < other); }
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
        int new_minute = total_minutes % 60;
        int total_hours = total_minutes / 60;
        int new_hour = total_hours % 24;
        int extra_days = total_hours / 24;

        Date new_date = date;
        new_date.day += extra_days;

        // Handle month overflow: June=30d, July=31d, August=31d, September=30d
        // With at most 72-hour travel, at most one month boundary is crossed.
        auto days_in_month = [](int m) -> int {
            if (m == 6 || m == 9) return 30;
            if (m == 7 || m == 8) return 31;
            return 31; // fallback
        };
        while (new_date.day > days_in_month(new_date.month))
        {
            new_date.day -= days_in_month(new_date.month);
            new_date.month++;
        }

        return AccurateTime(new_date, Time(new_hour, new_minute));
    }
    operator std::string() const
    {
        char buffer[20];
        std::snprintf(buffer, sizeof(buffer), "%02d-%02d %02d:%02d", date.month, date.day, time.hour, time.minute);
        return std::string(buffer);
    }
    bool operator<(const AccurateTime &other) const
    {
        if (date != other.date)
            return date < other.date;
        if (time.hour != other.time.hour)
            return time.hour < other.time.hour;
        return time.minute < other.time.minute;
    }
    bool operator==(const AccurateTime &other) const
    {
        return date == other.date && time.hour == other.time.hour && time.minute == other.time.minute;
    }
    bool operator!=(const AccurateTime &other) const { return !(*this == other); }
    bool operator<=(const AccurateTime &other) const { return *this < other || *this == other; }
    bool operator>(const AccurateTime &other) const { return !(*this <= other); }
    bool operator>=(const AccurateTime &other) const { return !(*this < other); }
    int operator-(const AccurateTime &other) const
    {
        // Convert dates to absolute day count for correct cross-month arithmetic
        // June=30d, July=31d, August=31d, September=30d
        auto to_abs_days = [](const Date &d) -> int {
            int days = 0;
            for (int m = 6; m < d.month; ++m)
            {
                if (m == 6 || m == 9) days += 30;
                else if (m == 7 || m == 8) days += 31;
            }
            return days + d.day;
        };
        int day_diff = to_abs_days(date) - to_abs_days(other.date);
        int time_diff = (time.hour - other.time.hour) * 60 + (time.minute - other.time.minute);
        return day_diff * 1440 + time_diff;
    }
};
#endif // TIME_HPP
