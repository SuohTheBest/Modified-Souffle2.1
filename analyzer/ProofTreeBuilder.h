#pragma once

#include "iostream"
#include "map"
#include "string"
#include "unordered_map"
#include "vector"
#include <fstream>
#include <set>
#include <unordered_set>
#include <algorithm>
#include "regex"
#include "string"
#include <cassert>
#include <cstring>

namespace modified_souffle {
	struct Tuple {
		size_t rule_id;
		std::string name;
		size_t size;
		size_t *parent_node;
		bool is_root;  // 通过input创造的tuple没有父节点

		Tuple(bool is_root, std::string &name)
				:
				name(name), parent_node(nullptr), is_root(is_root), rule_id(0), size(0) {};

		Tuple(bool is_root, std::string &name, size_t relation_id, size_t size)
				:
				rule_id(relation_id), name(name), is_root(is_root), size(size) {
			parent_node = (size_t *) malloc(size * sizeof(size_t));
		}

		~Tuple() {
			if (parent_node != nullptr) free(parent_node);
		}
	};

	struct RelationCount {
		std::string name;
		size_t pr;
		size_t fr;

		explicit RelationCount(std::string &relation) :
				name(relation), pr(0), fr(0) {};
	};

	class correctTupleExtractor {
	public:
		correctTupleExtractor(const char *path) {
			this->path = path;
			extract();
		}

		/** 提取的正确tuple */
		std::unordered_set<std::string> tuple_list;

	private:
		void extract();

		const char *path;
		bool flag;
		std::string set_name;
		std::vector<std::string> output_set;
		std::ifstream is;
	};

	class proofTreeBuilder {
	public:
		proofTreeBuilder(const char *path) {
			is.open(path);
			set_pattern.assign((R"(\s(\w*?)(?=\())"));
			tuple_pattern.assign((R"(\([\w,"]*\))"));
			build();
		}

		std::vector<Tuple *> tuple_list;
		std::vector<RelationCount> relation_list;
		std::vector<std::string> output_set;

	private:
		bool readALine();

		void build() {
			while (readALine());
			tuple_map.clear();
			relation_map.clear();
		};

		bool is_relation = false;
		size_t curr_tupleId = 0;
		/** tuple要被添加到的set */
		std::string curr_setName;
		/** 当前正在应用的规则 */
		std::string curr_relation;
		size_t curr_relationId = 0;
		std::unordered_map<std::string, size_t> tuple_map;
		std::unordered_map<std::string, size_t> relation_map;
		std::regex set_pattern;
		std::regex tuple_pattern;
		std::ifstream is;
	};
}  // namespace modified_souffle
