/**
 * @file Modify.h
 * 通过跟踪engine的执行打印出集合元素的变化，从而得到proof tree
 */
#pragma once
#include "cassert"
#include "iostream"
#include "sstream"
#include "string"
#include "vector"
#include <map>
#include "souffle/SymbolTable.h"

namespace modified_souffle {

/**
 * @class set_data
 * @brief 用于存储souffle中集合的变化
 */
class set_data {
public:
    set_data() = default;
    /** @param target_set 目标集合
     * @param tuple 要添加的元素
     * @brief 将元素和来源细节添加到目标集合中
     * */
    void insert_tuple(const std::string& target_set, const std::string& tuple, const std::string& detail);
    /**
     * @param source_set 源集合
     * @param target_set 目标集合
     * @brief 将源集合合并至目标集合中
     */
    void merge_set(const std::string& source_set,const std::string& target_set);
    /**
     * @brief 展示集合的变化
     */
    void show(std::ostream& os);
    /**
     * @brief 清空存储的所有集合
     */
    void clear();
    friend class TupleDataAnalyzer;

private:
    size_t counter = 0;
    std::map<std::string, size_t> set_index;
    std::vector<std::vector<std::string>> set;
    std::vector<std::vector<std::string>> detail_data;
};
/**
 * @class TupleScanManager
 * @brief 处理扫描tuple的嵌套循环，返回当前正在扫描的tuples
 */
class TupleScanManager {
public:
    explicit TupleScanManager(int depth) : max_loop_depth(depth) {
        scan_result.resize(max_loop_depth + 1);
    };
    /**
     * @brief 进入下一级循环
     */
    void enter_loop() {
        curr_loop_index += 1;
        assert(curr_loop_index <= max_loop_depth);
    }
    /**
     * @brief 结束当前循环，返回上一级循环
     */
    void exit_loop() {
        curr_loop_index -= 1;
        assert(curr_loop_index <= max_loop_depth);
    }
    /**
     * @brief 读取当前正在扫描的tuple并进行存储
     * @param tuple 当前正在扫描，且已被两种decode处理过的tuple
     */
    void scan_tuple(const std::string& tuple);
    /**
     * @brief 获得当前正在扫描的tuple的集合
     * @return 当前正在扫描的tuple的集合形成的详细信息
     */
    std::string read_tuple();

private:
    std::vector<std::string> scan_result;
    size_t curr_loop_index = 0;
    size_t max_loop_depth;
};

/**
 * @class InfoOrderManager
 * @brief 管理IndexScan中出现的view中tuple的order
 */
class InfoOrderManager {
public:
    InfoOrderManager() = default;
    /**
     * @brief 增加一种新的order
     * @param viewId 视图id
     * @param order 对应的order
     */
    void add_order(size_t viewId, std::vector<size_t>& order);
    /**
     * @brief 根据视图id返回对应的order
     * @param viewId 视图id
     * @return 对应的order
     */
    std::vector<size_t>& get_order(size_t viewId);

private:
    std::map<size_t, std::vector<size_t>> order_map;
};

/**
 * @class TupleDataAnalyzer
 * @brief 接收引擎执行传递的数据，处理后输出直观的结果
 */
class TupleDataAnalyzer {
public:
    explicit TupleDataAnalyzer(souffle::SymbolTable* symbolTable,std::ostream& os=std::cout);
    TupleDataAnalyzer(const std::string& output_path,souffle::SymbolTable* symbolTable);
    ~TupleDataAnalyzer();
    /**
     * @brief 从输入流中读取一行，并进行解读
     * @return 是否成功读取数据
     */
    bool parse();
    TupleDataAnalyzer& operator<<(const std::string& s);
    TupleDataAnalyzer& operator<<(std::ostream& (*manipulator)(std::ostream&));
    TupleDataAnalyzer& operator<<(const int& s);

private:
    std::string decodeTupleWithAssignedData(std::string tuple);
    std::string decodeTupleWithAssignedData(std::vector<size_t>& tuple);
    void decodeTupleByOrder(std::vector<size_t>& tuple, std::vector<size_t>& order);
    std::string curr_insertSet;
    std::string curr_scanSet;
    std::ostream* os;
    std::vector<std::size_t> curr_order;
    std::stringstream tuple_data;
    set_data set;
    TupleScanManager* scan_manager = nullptr;
    InfoOrderManager* order_manager = nullptr;
    souffle::SymbolTable* symbolTable;
    bool is_relation = false;
    bool is_skip_loop =false;
};

}  // namespace modified_souffle
