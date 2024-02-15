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
		size_t rule_id;
		std::string name;
		size_t size;
		size_t *parent_node;
		bool is_root;  // 通过input创造的tuple没有父节点
		Tuple() = default;

		Tuple(bool is_root, std::string &name, size_t id)
				:
				id(id), name(name), parent_node(nullptr), is_root(is_root) {};

		Tuple(bool is_root, std::string &name, size_t id, size_t relation_id, size_t size)
				:
				id(id), rule_id(relation_id), name(name), is_root(is_root), size(size) {
			parent_node = (size_t *) malloc(size * sizeof(size_t));
		}

		~Tuple() {
			if (parent_node != nullptr)free(parent_node);
		}
	};

	class proofTreeBuilder {
	public:
		proofTreeBuilder(const char *path) {
			is.open(path);
		}

		void build() {
			while (readALine());
		};
		std::vector<Tuple *> tuple_list;
		std::vector<std::string> relation_list;
		std::vector<std::string> output_set;
	private:
		bool readALine();

		bool is_relation = false;
		size_t curr_tupleId = 0;
		/** tuple要被添加到的set */
		std::string curr_setName = "";
		/** 当前正在应用的规则 */
		std::string curr_relation = "";
		size_t curr_relationId = 0;
		std::unordered_map<std::string, size_t> tuple_map;
		std::unordered_map<std::string, size_t> relation_map;
		std::ifstream is;
	};
}  // namespace modified_souffle
