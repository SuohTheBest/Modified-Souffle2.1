#pragma once
#include "iostream"
#include "map"
#include "string"
#include "unordered_map"
#include "vector"
#include <fstream>
#include <set>
namespace modified_souffle {
struct Tuple {
    size_t id;
    std::string rule;
    std::string name;
    size_t* parent_node;
    bool is_root;  // 通过input创造的tuple没有父节点
    Tuple() = default;
    Tuple(bool is_root, std::string& name, size_t id)
            : id(id), name(name), parent_node(nullptr), is_root(is_root){};
    Tuple(bool is_root, std::string& name, size_t id, std::string& relation, size_t size)
            : id(id), rule(relation), name(name), is_root(is_root) {
        parent_node = (size_t*)malloc(size * sizeof(size_t));
    }
    ~Tuple() {
        free(parent_node);
    }
};

class proofTreeBuilder {
public:
    proofTreeBuilder(const char *path) {
        is.open(path);
    }

    void build() {
        while (readALine())
            ;
    };

private:
    bool readALine();
    bool is_relation = false;
    size_t curr_tupleId = 0;
    std::string curr_setName = "";
    std::string curr_relation = "";
    std::vector<Tuple*> tuple_list;
    std::unordered_map<std::string,size_t> hashmap;
    std::set<std::string> relation_list;
    std::ifstream is;
};
}  // namespace modified_souffle
