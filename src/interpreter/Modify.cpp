#include "Modify.h"
#include "fstream"

enum class opType {
    debug,
    insert_target,
    insert,
    swap,
    clear,
    scan_eval,
    scan_index,
    scan_order,
    info_order,
    scan_target,
    end_scan,
    output,
    other,
};

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

opType getOperationType(const std::string& operation) {
    if (operation == "DEBUG")
        return opType::debug;
    else if (operation == "INSERT_TARGET")
        return opType::insert_target;
    else if (operation == "INSERT")
        return opType::insert;
    else if (operation == "SWAP")
        return opType::swap;
    else if (operation == "CLEAR")
        return opType::clear;
    else if (operation == "SCAN_EVAL")
        return opType::scan_eval;
    else if (operation == "SCAN_INDEX")
        return opType::scan_index;
    else if (operation == "SCAN_ORDER")
        return opType::scan_order;
    else if (operation == "INFO_ORDER")
        return opType::info_order;
    else if (operation == "SCAN_TARGET")
        return opType::scan_target;
    else if (operation == "END_SCAN")
        return opType::end_scan;
    else if (operation == "OUTPUT")
        return opType::output;
    else
        return opType::other;
}

bool modified_souffle::TupleDataAnalyzer::parse() {
    std::string line;
    if (!std::getline(tuple_data, line)) return false;
    // (*os) << "###" << line << std::endl;
    std::cout<<line<<std::endl;
    size_t split_pos = line.find(' ');
    std::string operation = line.substr(0, split_pos);
    std::string data(line.begin() + split_pos + 1, line.end() - 1);
    opType type = getOperationType(operation);
    switch (type) {
        case opType::debug: {
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
        case opType::insert_target: {
            curr_insertSet = data;
            break;
        }
        case opType::insert: {
            if (curr_insertSet.empty() || is_skip_loop) break;
            size_t op_pos = data.find(':');
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
        case opType::swap: {
            size_t op_pos = data.find(' ');
            std::string set1 = data.substr(0, op_pos);
            std::string set2 = data.substr(op_pos + 1);
            if (set1[0] == '@' && set2[0] == '@')
                break;
            else {
                // (*os) << "发生了意料外的交换：" << set1 << ' ' << set2 << std::endl;
            }
            break;
        }
        case opType::clear: {
            if (data.empty() || data[0] == '@')
                break;
            else {
                // (*os) << "发生了意料外的清除：" << data << std::endl;
            }
            break;
        }
        case opType::scan_order: {
            curr_order.clear();
            curr_order = stringToTuple(data);
            break;
        }
        case opType::scan_eval: {
            std::vector<size_t> tuple = stringToTuple(data);
            decodeTupleByOrder(tuple, curr_order);
            scan_manager->scan_tuple(decodeTupleWithAssignedData(tuple));
            break;
        }
        case opType::info_order: {
            size_t op_pos = data.find(' ');
            std::string sub1 = data.substr(0, op_pos);
            std::string sub2 = data.substr(op_pos + 1);
            size_t viewId = stoi(sub1);
            std::vector<size_t> order = stringToTuple(sub2);
            order_manager->add_order(viewId, order);
            break;
        }
        case opType::scan_target: {
            scan_manager->enter_loop();
            curr_scanSet = data;
            break;
        }
        case opType::end_scan: {
            scan_manager->exit_loop();
            is_skip_loop = false;
            break;
        }
        case opType::scan_index: {
            size_t op_pos = data.find(' ');
            size_t viewId = stoi(data.substr(0, op_pos));
            std::string tuple_str = data.substr(op_pos + 1);
            std::vector<size_t> tuple = stringToTuple(tuple_str);
            decodeTupleByOrder(tuple, order_manager->get_order(viewId));
            scan_manager->scan_tuple(decodeTupleWithAssignedData(tuple));
            break;
        }
        case opType::output: {
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
            token = symbolTable->decode(number);
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
        ans = ans + symbolTable->decode(i) + ",";
    }
    *(ans.end() - 1) = ')';
    return ans;
}

modified_souffle::TupleDataAnalyzer::TupleDataAnalyzer(souffle::SymbolTable* symbolTable, std::ostream& os) {
    this->os = &os;
    this->symbolTable = symbolTable;
}

modified_souffle::TupleDataAnalyzer::TupleDataAnalyzer(
        const std::string& output_path, souffle::SymbolTable* symbolTable) {
    this->symbolTable = symbolTable;
    if (output_path.empty())
        this->os = &std::cout;
    else
        this->os = new std::ofstream(output_path);
}

void modified_souffle::set_data::insert_tuple(
        const std::string& target_set, const std::string& tuple, const std::string& detail) {
    auto it = set_index.find(target_set);
    if (it != set_index.end()) {
        size_t index = it->second;
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
