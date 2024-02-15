#include "ProofTreeBuilder.h"
#include "regex"
#include "string"
#include <cassert>
#include <cstring>

bool modified_souffle::proofTreeBuilder::readALine() {
    std::string line;
    if (!std::getline(is, line)) return false;
    if (line[0] == '+')  // 向集合中添加tuple
    {
        std::sregex_iterator end;
        std::regex tuple_pattern(R"(\([\w,]*\))");
        std::smatch matches;
        std::string tuple;
        if (!is_relation)  // 是叶子节点
        {
            std::sregex_iterator iterator(line.begin(), line.end(), tuple_pattern);
            assert(iterator != end);
            matches = *iterator;
            tuple = matches.str() + "@" + curr_setName;
            tuple_list.push_back(new Tuple(true, tuple, curr_tupleId));
            tuple_map[tuple] = curr_tupleId;
            curr_tupleId++;
        } else {  // 非叶子节点，要加载父节点数据
            std::sregex_iterator iterator(line.begin(), line.end(), tuple_pattern);
            assert(iterator != end);
            matches = *iterator++;
            tuple = matches.str() + "@" + curr_setName;
            // 解析规则中的set
            std::regex set_pattern(R"(\s(\w*?)(?=\())");
            std::smatch set_matches;
            std::sregex_iterator set_iterator(curr_relation.begin(), curr_relation.end(), set_pattern);
            std::vector<std::string> set_list;
            while (set_iterator != end) {
                set_matches = *set_iterator++;
                set_list.push_back(set_matches.str());
            }
            // 处理添加的tuple
            Tuple* tuple1 = new Tuple(false, tuple, curr_tupleId, curr_relationId, set_list.size());
            int i = 0;
            while (iterator != end) {
                matches = *iterator++;
                tuple = matches.str() + "@" + set_list[i].substr(1);
                auto it = tuple_map.find(tuple);
                assert(it != tuple_map.end());
                tuple1->parent_node[i] = it->second;
                i++;
            }
            tuple_list.push_back(tuple1);
            tuple_map[tuple1->name] = curr_tupleId;
            curr_tupleId++;
        }
        return true;
    }
    auto op_pos = line.find(':');
    if (op_pos == std::string::npos) return true;
    std::string operation = line.substr(0, op_pos);
    std::string data = line.substr(op_pos + 1);
    if (operation == "read input")
        is_relation = false;
    else if (operation == "apply rules") {
        is_relation = true;
        op_pos = data.find('.');
        assert(op_pos != std::string::npos && "op_pos can't be npos.");
        curr_relation = data.substr(0, op_pos);
        auto it = relation_map.find(curr_relation);
        if (it == relation_map.end())  // 当前应用的是新的relation
        {
            relation_map[curr_relation] = relation_list.size();
            curr_relationId = relation_list.size();
            relation_list.push_back(curr_relation);
        } else {
            curr_relationId = it->second;
        }
    } else if (operation == "output set") {
        output_set.push_back(data);
    } else {
        curr_setName = operation;
    }
    return true;
}
