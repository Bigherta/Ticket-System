#ifndef TRAIN_HPP
#define TRAIN_HPP
#include <string>
#include <cstring>
#include "../BPlusTree/BPT.hpp"
#include "../BPlusTree/BPT_MemoryRiver.hpp"
#include "../Library/string_key.hpp"
#include "../Grammar/Token.hpp"
#include "../Library/set.hpp"
#include "../Library/vector.hpp"
#include "time.hpp"
class OrderManager;
class TrainManager;
class Train
{
    friend class TrainManager;
    friend class OrderManager;
private:
    char train_id[21]{};
    int station_num;
    char stations[101][41]{};
    int total_seat_num;
    int prices_prefix[101]{};
    Time start_time;
    int arrive_time_offset[101]{};
    int depart_time_offset[101]{};
    Date start_date;
    Date end_date;
    char type;
    bool is_released = false;

public:
    Train() = default;
    void addStation(const sjtu::vector<std::string> &stations_list)
    {
        for (int i = 0; i < station_num; ++i)
        {
            std::strncpy(stations[i], stations_list[i].c_str(), 40);
            stations[i][40] = '\0';
        }
    }
    void addPrices(const sjtu::vector<std::string> &prices)
    {
        for (int i = 1; i <= station_num - 1; ++i)
        {
            prices_prefix[i] = prices_prefix[i - 1] + std::stoi(prices[i - 1]);
        }
    }
    void addTimeoffset(const sjtu::vector<std::string> &travel_time, const sjtu::vector<std::string> &stopover_time)
    {
        arrive_time_offset[0] = 0;
        depart_time_offset[0] = 0;
        for (int i = 1; i < station_num; ++i)
        {
            int travel = std::stoi(travel_time[i - 1]);
            arrive_time_offset[i] = depart_time_offset[i - 1] + travel;
            if (i < station_num - 1)
            {
                int stopover = std::stoi(stopover_time[i - 1]);
                depart_time_offset[i] = arrive_time_offset[i] + stopover;
            }
            else
            {
                depart_time_offset[i] = arrive_time_offset[i];
            }
        }
    }
};
class TrainManager
{
    friend class Train;
    friend class OrderManager;
public:
    /**
     * @brief 构造函数，初始化 TrainManager（载入/建立索引与数据库）
     */
    TrainManager();
    ~TrainManager();

    /**
     * @brief 添加一列车次记录
     * @param train_id 列车编号
     * @param station_num 站点数量（字符串形式）
     * @param seat_num 座位数（字符串形式）
     * @param stations 站点列表（以'|'分隔）
     * @param prices 票价列表（以'|'分隔）
     * @param start_time 发车时间（字符串）
     * @param travel_time 行车时间列表（以'|'分隔）
     * @param stopover_time 停站时间列表（以'|'分隔，二站时为"_"）
     * @param sale 售卖日期区间（字符串）
     * @param type 列车类型（字符串）
     * @return 返回操作结果字符串
     */
    std::string addTrain(const std::string &train_id, const std::string &station_num, const std::string &seat_num,
                         const std::string &stations, const std::string &prices, const std::string &start_time,
                         const std::string &travel_time, const std::string &stopover_time, const std::string &sale,
                         const std::string &type);

    /**
     * @brief 删除未发布的列车记录（根据 train_id）
     * @param train_id 待删除的列车编号
     * @return 返回操作结果字符串（成功或失败原因）
     */
    std::string deleteTrain(const std::string &train_id);

    /**
     * @brief 发布列车（将列车从未发布状态转为可查询/售票状态）
     * @param train_id 要发布的列车编号
     * @return 返回操作结果字符串
     */
    std::string releaseTrain(const std::string &train_id);

    /**
     * @brief 查询列车在指定日期的运行信息
     * @param train_id 查询的列车编号
     * @param date 查询日期（字符串）
     * @return 返回查询结果字符串
     */
    std::string queryTrain(const std::string &train_id, const std::string &date);

    /**
     * @brief 查询指定日期从某站到某站的列车信息
     * @param from 出发站
     * @param to 目的站
     * @param date 查询日期（字符串）
     * @param priority 排序优先级（"time" or "cost"）
     * @return 返回查询结果字符串
     */
    std::string queryTicket(const std::string &from, const std::string &to, const std::string &date,
                            const std::string priority);

    /**
     * @brief 查询指定日期从某站到某站的换乘方案
     * @param from 出发站
     * @param to 目的站
     * @param date 查询日期（字符串）
     * @param priority 排序优先级（"time" or "cost"）
     * @return 返回查询结果字符串
     */
    std::string queryTransfer(const std::string &from, const std::string &to, const std::string &date,
                              const std::string priority);
    /**
     * @brief 处理列车相关指令
     * @param tokens 解析后的指令参数列表
     * @return 执行结果字符串
     */
    std::string handleTrainCommand(TokenStream &tokens);
    /**
     * @brief 清空列车数据与索引
     */
    void clean();

private:
    BPT<sjtu::StringKey<21>, int> trainIndex;
    BPT<sjtu::StringKey<41>, sjtu::pair<int, int>> station_train_mapping;
    struct SeatStatus
    {
        int train_addr;
        int station_index;
        Date date;
        bool operator<(const SeatStatus &other) const
        {
            if (train_addr != other.train_addr)
                return train_addr < other.train_addr;
            if (date != other.date)
                return date < other.date;
            return station_index < other.station_index;
        }
        bool operator==(const SeatStatus &other) const
        {
            return train_addr == other.train_addr && date == other.date && station_index == other.station_index;
        }
        bool operator!=(const SeatStatus &other) const { return !(*this == other); }
        bool operator<=(const SeatStatus &other) const { return !(other < *this); }
    };
    BPT<SeatStatus, int> seat_manager;
    MemoryRiver<Train> trainDatabase;
    BufferPoolManager<Train> *trainBufferPool;
    sjtu::set<std::string> releasedTrains;
};
#endif // TRAIN_HPP
