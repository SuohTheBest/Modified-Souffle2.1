#include "ProofTreeBuilder.h"

using namespace modified_souffle;

int main()
{
	proofTreeBuilder correct(R"(D:\souffle\cmake-build-souffle\src\souffle-analyze-data\output_0)");
	correct.build();
    proofTreeBuilder wrong(R"(D:\souffle\cmake-build-souffle\src\souffle-analyze-data\output_1)");
    wrong.build();

	return 0;
}