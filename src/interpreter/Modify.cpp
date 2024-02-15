#include "Modify.h"
#include "fstream"

#define DEBUG 0
#define INSERT_TARGET 1
#define INSERT 2
#define ASSIGN 3
#define SWAP 4
#define CLEAR 5
#define SCAN_EVAL 6
#define SCAN_INDEX 7
#define SCAN_ORDER 8
#define INFO_ORDER 9
#define SCAN_TARGET 10
#define END_SCAN 11
#define OUTPUT 12

int countSubstringOccurrences(const std::string& str, const std::string& sub) {
    int count = 0;
    size_t pos = 0;
    while ((pos = str.find(sub, pos)) != std::string::npos) {
        count++;
        pos += sub.length();
    }
    return count;
}

std::vector<size_t> stringToTuple(const std::string& str) {
    assert((str[0] == '[' || str[0] == '(') && (str[str.size() - 1] == ']' || str[str.size() - 1] == ')'));
    std::stringstream ss(str.substr(1, str.size() - 1));
    std::string temp;
    std::vector<size_t> tuple;
    while (std::getline(ss, temp, ',')) {
        try {
            size_t number = std::stoi(temp);
            tuple.push_back(number);
        } catch (const std::exception& e) {
            std::cerr << "tuple格式错误.";
            std::cerr << e.what();
        }
    }
    return tuple;
}

int getOperationType(const std::string& operation) {
    if (operation == "DEBUG")
        return DEBUG;
    else if (operation == "INSERT_TARGET")
        return INSERT_TARGET;
    else if (operation == "INSERT")
        return INSERT;
    else if (operation == "ASSIGN")
        return ASSIGN;
    else if (operation == "SWAP")
        return SWAP;
    else if (operation == "CLEAR")
        return CLEAR;
    else if (operation == "SCAN_EVAL")
        return SCAN_EVAL;
    else if (operation == "SCAN_INDEX")
        return SCAN_INDEX;
    else if (operation == "SCAN_ORDER")
        return SCAN_ORDER;
    else if (operation == "INFO_ORDER")
        return INFO_ORDER;
    else if (operation == "SCAN_TARGET")
        return SCAN_TARGET;
    else if (operation == "END_SCAN")
        return END_SCAN;
    else if (operation == "OUTPUT")
        return OUTPUT;
    else
        return INT8_MAX;
}

bool modified_souffle::TupleDataAnalyzer::parse() {
    std::string line;
    if (!std::getline(tuple_data, line)) return false;
    // (*os) << "###" << line << std::endl;
    int split_pos = line.find(' ');
    std::string operation = line.substr(0, split_pos);
    std::string data(line.begin() + split_pos + 1, line.end() - 1);
    int type = getOperationType(operation);
    switch (type) {
        case DEBUG: {
            if (set.counter != 0) {
                set.show(*os);
                set.clear();
            }
            delete scan_manager;
            delete order_manager;
            scan_manager = nullptr;
            order_manager = nullptr;
            is_relation = data.find(":-") != std::string::npos;
            if (is_relation) {
                scan_manager = new TupleScanManager(1 + countSubstringOccurrences(data, "), "));
                order_manager = new InfoOrderManager();
                (*os) << "apply rules:" << data << std::endl;
            } else
                (*os) << "read input:" << data << std::endl;
            break;
        }
        case INSERT_TARGET: {
            curr_insertSet = data;
            break;
        }
        case INSERT: {
            if (curr_insertSet.empty() || is_skip_loop) break;
            int op_pos = data.find(":");
            std::string tuple = data.substr(op_pos + 2);
            if (is_relation) {
                std::string detail = scan_manager->read_tuple();
                if (detail == "_") {
                    if (curr_insertSet[0] != '@') {
                        set.merge_set(curr_scanSet, curr_insertSet);
                        is_skip_loop = true;
                    } else
                        break;
                } else
                    set.insert_tuple(curr_insertSet, decodeTupleWithAssignedData(tuple), detail);
            } else
                set.insert_tuple(curr_insertSet, decodeTupleWithAssignedData(tuple), "");
            break;
        }
        case ASSIGN: {
            try {
                int op_pos = data.find("=");
                size_t number = std::stoi(data.substr(0, op_pos));
                if (number >= assign_data.size()) {
                    assign_data.resize(2 * number + 1);
                }
                assign_data[number] = data.substr(op_pos + 2);
            } catch (const std::exception& e) {
                std::cerr << "处理赋值语句时发生错误." << std::endl;
                std::cerr << e.what();
            }
            break;
        }
        case SWAP: {
            int op_pos = data.find(' ');
            std::string set1 = data.substr(0, op_pos);
            std::string set2 = data.substr(op_pos + 1);
            if (set1[0] == '@' && set2[0] == '@')
                break;
            else {
                // (*os) << "发生了意料外的交换：" << set1 << ' ' << set2 << std::endl;
            }
            break;
        }
        case CLEAR: {
            if (data.empty() || data[0] == '@')
                break;
            else {
                // (*os) << "发生了意料外的清除：" << data << std::endl;
            }
            break;
        }
        case SCAN_ORDER: {
            curr_order.clear();
            curr_order = stringToTuple(data);
            break;
        }
        case SCAN_EVAL: {
            std::vector<size_t> tuple = stringToTuple(data);
            decodeTupleByOrder(tuple, curr_order);
            scan_manager->scan_tuple(decodeTupleWithAssignedData(tuple));
            break;
        }
        case INFO_ORDER: {
            int op_pos = data.find(' ');
            std::string sub1 = data.substr(0, op_pos);
            std::string sub2 = data.substr(op_pos + 1);
            size_t viewId = stoi(sub1);
            std::vector<size_t> order = stringToTuple(sub2);
            order_manager->add_order(viewId, order);
            break;
        }
        case SCAN_TARGET: {
            scan_manager->enter_loop();
            curr_scanSet = data;
            break;
        }
        case END_SCAN: {
            scan_manager->exit_loop();
            is_skip_loop = false;
            break;
        }
        case SCAN_INDEX: {
            int op_pos = data.find(' ');
            size_t viewId = stoi(data.substr(0, op_pos));
            std::string tuple_str = data.substr(op_pos + 1);
            std::vector<size_t> tuple = stringToTuple(tuple_str);
            decodeTupleByOrder(tuple, order_manager->get_order(viewId));
            scan_manager->scan_tuple(decodeTupleWithAssignedData(tuple));
            break;
        }
        case OUTPUT: {
            (*os) << "output set:" << data << std::endl;
            break;
        }
        default: break;
    }
    return true;
}

std::string modified_souffle::TupleDataAnalyzer::decodeTupleWithAssignedData(const std::string tuple) {
    std::stringstream ss(tuple.substr(1, tuple.size() - 1));
    std::string token;
    std::string ans = "(";
    while (std::getline(ss, token, ',')) {
        try {
            size_t number = std::stoi(token);
            token = assign_data[number];
            ans = ans + token + ",";
        } catch (const std::exception& e) {
            std::cerr << "将赋值应用在tuple上时发生错误.";
            std::cerr << e.what();
        }
    }
    *(ans.end() - 1) = ')';
    return ans;
}

modified_souffle::TupleDataAnalyzer& modified_souffle::TupleDataAnalyzer::operator<<(const std::string& s) {
    tuple_data << s << ' ';
    return *this;
}

modified_souffle::TupleDataAnalyzer& modified_souffle::TupleDataAnalyzer::operator<<(const int& s) {
    tuple_data << s << ' ';
    return *this;
}

modified_souffle::TupleDataAnalyzer& modified_souffle::TupleDataAnalyzer::operator<<(
        std::ostream& (*manipulator)(std::ostream&)) {
    tuple_data << manipulator;
    return *this;
}

modified_souffle::TupleDataAnalyzer::~TupleDataAnalyzer() {
    while (parse())
        (*os) << "case 2" << std::endl;
}

void modified_souffle::TupleDataAnalyzer::decodeTupleByOrder(
        std::vector<size_t>& tuple, std::vector<size_t>& order) {
    assert(tuple.size() == order.size() && tuple.size() != 0);
    std::vector<size_t> temp;
    temp.resize(tuple.size());
    for (size_t i = 0; i < tuple.size(); ++i) {
        temp[order[i]] = tuple[i];
    }
    std::swap(temp, tuple);
}

std::string modified_souffle::TupleDataAnalyzer::decodeTupleWithAssignedData(std::vector<size_t>& tuple) {
    std::string ans = "(";
    for (unsigned long i : tuple) {
        ans = ans + assign_data[i] + ",";
    }
    *(ans.end() - 1) = ')';
    return ans;
}

modified_souffle::TupleDataAnalyzer::TupleDataAnalyzer(std::ostream& os) {
    this->os = &os;
}

modified_souffle::TupleDataAnalyzer::TupleDataAnalyzer(const std::string& output_path) {
    if (output_path == "")
        this->os = &std::cout;
    else
        this->os = new std::ofstream(output_path);
}

void modified_souffle::set_data::insert_tuple(
        const std::string& target_set, const std::string& tuple, const std::string& detail) {
    auto it = set_index.find(target_set);
    if (it != set_index.end()) {
        int index = it->second;
        set[index].push_back(tuple);
        detail_data[index].push_back(detail);
    } else {
        set_index[target_set] = counter;
        if (counter >= set.size()) {
            set.resize(2 * counter + 1);
            detail_data.resize(2 * counter + 1);
        }
        set[counter].push_back(tuple);
        detail_data[counter].push_back(detail);
        counter++;
    }
}

void modified_souffle::set_data::show(std::ostream& os) {
    for (auto& i : set_index) {
        if (i.first[0] == '@') continue;
        os << i.first << ":" << std::endl;
        for (size_t j = 0; j < set[i.second].size(); ++j) {
            os << "+" << set[i.second][j] << " " << detail_data[i.second][j] << " " << std::endl;
        }
        os << std::endl;
    }
    os << std::endl;
}

void modified_souffle::set_data::clear() {
    set_index.clear();
    for (auto& i : set) {
        i.clear();
    }
    for (auto& i : detail_data) {
        i.clear();
    }
    set.clear();
    detail_data.clear();
    counter = 0;
}

void modified_souffle::set_data::merge_set(const std::string& source_set, const std::string& target_set) {
    auto it_src = set_index.find(source_set);
    assert(it_src != set_index.end());
    auto it_dst = set_index.find(target_set);
    if (it_dst == set_index.end()) {
        set_index[target_set] = counter;
        if (counter >= set.size()) {
            set.resize(2 * counter + 1);
            detail_data.resize(2 * counter + 1);
        }
        counter++;
        it_dst = set_index.find(target_set);
    }
    for (size_t i = 0; i < set[it_src->second].size(); ++i) {
        set[it_dst->second].push_back(set[it_src->second][i]);
        detail_data[it_dst->second].push_back(detail_data[it_src->second][i]);
    }
}

std::string modified_souffle::TupleScanManager::read_tuple() {
    if (curr_loop_index == max_loop_depth) {
        std::string s = " from:[";
        for (size_t i = 1; i < scan_result.size(); ++i) {
            s = s + scan_result[i] + ",";
        }
        *(s.end() - 1) = ']';
        return s;
    } else
        return "_";
}

void modified_souffle::TupleScanManager::scan_tuple(const std::string& tuple) {
    scan_result[curr_loop_index] = tuple;
}

void modified_souffle::InfoOrderManager::add_order(size_t viewId, std::vector<size_t>& order) {
    order_map[viewId] = order;
}

std::vector<size_t>& modified_souffle::InfoOrderManager::get_order(size_t viewId) {
    auto it = order_map.find(viewId);
    if (it != order_map.end()) {
        return it->second;
    } else {
        std::cerr << "在使用view id寻找order时发生错误." << std::endl;
        throw std::runtime_error("view id invalid.");
    }
}
