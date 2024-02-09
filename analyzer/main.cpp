#include "ProofTreeBuilder.h"

using namespace modified_souffle;

int main()
{
	proofTreeBuilder builder(R"(D:\souffle\cmake-build-souffle\src\souffle-analyze-data\correct)");
	builder.build();
	return 0;
}