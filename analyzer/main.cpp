#include "ProofTreeBuilder.h"

using namespace modified_souffle;

void proofTreeTravel(proofTreeBuilder &builder, Tuple *tuple, bool is_correct) {
	if (tuple->is_root)return;
	if (is_correct)builder.relation_list[tuple->rule_id].pr++;
	else builder.relation_list[tuple->rule_id].fr++;
	for (size_t i = 0; i < tuple->size; ++i) {
		proofTreeTravel(builder, builder.tuple_list[tuple->parent_node[i]], is_correct);
	}
}

int main() {
	size_t p = 0;
	size_t f = 0;
	correctTupleExtractor correct(
			R"(D:\souffle-2.1\souffle-2.1\souffle-analyze-data\output_0)");
	proofTreeBuilder wrong(R"(D:\souffle-2.1\souffle-2.1\souffle-analyze-data\output_1)");
	for (Tuple *tuple: wrong.tuple_list) {
		// tuple是根节点，无需进行统计
		if (tuple->is_root) continue;
		std::string set_name = tuple->name.substr(tuple->name.find('@') + 1);
		// 是最终输出的tuple
		if (std::find(wrong.output_set.begin(), wrong.output_set.end(), set_name) != wrong.output_set.end()) {
			auto it = correct.tuple_list.find(tuple->name);
			// 存在于正确的tuple列表中，从tuple_list移除该tuple，节约后续查找时间
			if (it != correct.tuple_list.end()) {
				correct.tuple_list.erase(it);
				p++;
				proofTreeTravel(wrong, tuple, true);
			}
				// 不存在列表中，是错误的元组，进行计数
			else {
				f++;
				proofTreeTravel(wrong, tuple, false);
			}
		}
	}
	// 最终correct.tuple_list留下来的是缺失的元组
	std::cout << "P = " << p << "\tF = " << f << std::endl;
	for (RelationCount &relation: wrong.relation_list) {
		double op_val = relation.fr - relation.pr / (p + 1.0);
		std::cout << relation.name << "\t Pr = " << relation.pr << "\t Fr = " << relation.fr << "\t Op = " << op_val
				  << std::endl;
	}
	return 0;
}