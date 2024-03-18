/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file main.cpp
 *
 * Main driver for Souffle
 *
 ***********************************************************************/

#include "Global.h"
#include "ast/Node.h"
#include "ast/Program.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/PrecedenceGraph.h"
#include "ast/analysis/SCCGraph.h"
#include "ast/analysis/Type.h"
#include "ast/transform/AddNullariesToAtomlessAggregates.h"
#include "ast/transform/ComponentChecker.h"
#include "ast/transform/ComponentInstantiation.h"
#include "ast/transform/Conditional.h"
#include "ast/transform/ExecutionPlanChecker.h"
#include "ast/transform/ExpandEqrels.h"
#include "ast/transform/Fixpoint.h"
#include "ast/transform/FoldAnonymousRecords.h"
#include "ast/transform/GroundWitnesses.h"
#include "ast/transform/GroundedTermsChecker.h"
#include "ast/transform/IOAttributes.h"
#include "ast/transform/IODefaults.h"
#include "ast/transform/InlineRelations.h"
#include "ast/transform/MagicSet.h"
#include "ast/transform/MaterializeAggregationQueries.h"
#include "ast/transform/MaterializeSingletonAggregation.h"
#include "ast/transform/MinimiseProgram.h"
#include "ast/transform/NameUnnamedVariables.h"
#include "ast/transform/NormaliseGenerators.h"
#include "ast/transform/PartitionBodyLiterals.h"
#include "ast/transform/Pipeline.h"
#include "ast/transform/PragmaChecker.h"
#include "ast/transform/ReduceExistentials.h"
#include "ast/transform/RemoveBooleanConstraints.h"
#include "ast/transform/RemoveEmptyRelations.h"
#include "ast/transform/RemoveRedundantRelations.h"
#include "ast/transform/RemoveRedundantSums.h"
#include "ast/transform/RemoveRelationCopies.h"
#include "ast/transform/ReorderLiterals.h"
#include "ast/transform/ReplaceSingletonVariables.h"
#include "ast/transform/ResolveAliases.h"
#include "ast/transform/ResolveAnonymousRecordAliases.h"
#include "ast/transform/SemanticChecker.h"
#include "ast/transform/SimplifyAggregateTargetExpression.h"
#include "ast/transform/UniqueAggregationVariables.h"
#include "ast2ram/TranslationStrategy.h"
#include "ast2ram/UnitTranslator.h"
#include "ast2ram/provenance/TranslationStrategy.h"
#include "ast2ram/provenance/UnitTranslator.h"
#include "ast2ram/seminaive/TranslationStrategy.h"
#include "ast2ram/seminaive/UnitTranslator.h"
#include "ast2ram/utility/TranslatorContext.h"
#include "config.h"
#include "interpreter/Engine.h"
#include "interpreter/ProgInterface.h"
#include "parser/ParserDriver.h"
#include "ram/Node.h"
#include "ram/Program.h"
#include "ram/TranslationUnit.h"
#include "ram/transform/CollapseFilters.h"
#include "ram/transform/Conditional.h"
#include "ram/transform/EliminateDuplicates.h"
#include "ram/transform/ExpandFilter.h"
#include "ram/transform/HoistAggregate.h"
#include "ram/transform/HoistConditions.h"
#include "ram/transform/IfConversion.h"
#include "ram/transform/IfExistsConversion.h"
#include "ram/transform/Loop.h"
#include "ram/transform/MakeIndex.h"
#include "ram/transform/Parallel.h"
#include "ram/transform/ReorderConditions.h"
#include "ram/transform/ReorderFilterBreak.h"
#include "ram/transform/ReportIndex.h"
#include "ram/transform/Sequence.h"
#include "ram/transform/Transformer.h"
#include "ram/transform/TupleId.h"
#include "reports/DebugReport.h"
#include "reports/ErrorReport.h"
#include "souffle/RamTypes.h"
#include "souffle/profile/Tui.h"
#include "souffle/provenance/Explain.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/FileUtil.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include "souffle/utility/StringUtil.h"
#include "synthesiser/Synthesiser.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace modified_souffle {
int countFilesInDirectory(const std::string& path) {
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_regular_file(entry)) {
            count++;
        }
    }
    return count;
}

}  // namespace modified_souffle
namespace souffle {

/**
 * Executes a binary file.
 */
void executeBinary(const std::string& binaryFilename) {
    assert(!binaryFilename.empty() && "binary filename cannot be blank");

    // check whether the executable exists
    if (!isExecutable(binaryFilename)) {
        throw std::invalid_argument("Generated executable <" + binaryFilename + "> could not be found");
    }

    std::string ldPath;
    // run the executable
    if (Global::config().has("library-dir")) {
        for (const std::string& library : splitString(Global::config().get("library-dir"), ' ')) {
            ldPath += library + ':';
        }
        ldPath.pop_back();
        setenv("LD_LIBRARY_PATH", ldPath.c_str(), 1);
    }

    std::string exePath;
#ifdef __APPLE__
    // OSX does not pass on the environment from setenv so add it to the command line
    exePath = "DYLD_LIBRARY_PATH=\"" + ldPath + "\" ";
#endif
    exePath += binaryFilename;

    int exitCode = system(exePath.c_str());

    if (Global::config().get("dl-program").empty()) {
        remove(binaryFilename.c_str());
        remove((binaryFilename + ".cpp").c_str());
    }

    // exit with same code as executable
    if (exitCode != EXIT_SUCCESS) {
        exit(exitCode);
    }
}

/**
 * Compiles the given source file to a binary file.
 */
void compileToBinary(std::string compileCmd, const std::string& sourceFilename) {
    // add source code
    compileCmd += ' ';
    for (const std::string& path : splitString(Global::config().get("library-dir"), ' ')) {
        // The first entry may be blank
        if (path.empty()) {
            continue;
        }
        compileCmd += "-L" + path + ' ';
    }
    for (const std::string& library : splitString(Global::config().get("libraries"), ' ')) {
        // The first entry may be blank
        if (library.empty()) {
            continue;
        }
        compileCmd += "-l" + library + ' ';
    }

    compileCmd += sourceFilename;

    // run executable
    if (system(compileCmd.c_str()) != 0) {
        throw std::invalid_argument("failed to compile C++ source <" + sourceFilename + ">");
    }
}

int main(int argc, char** argv) {
    /* Time taking for overall runtime */
    auto souffle_start = std::chrono::high_resolution_clock::now();

    /* have all to do with command line arguments in its own scope, as these are accessible through the global
     * configuration only */
    try {
        std::stringstream header;
        header << "============================================================================" << std::endl;
        header << "souffle -- A datalog engine." << std::endl;
        header << "Usage: souffle [OPTION] FILE." << std::endl;
        header << "----------------------------------------------------------------------------" << std::endl;
        header << "Options:" << std::endl;

        std::stringstream footer;
        footer << "----------------------------------------------------------------------------" << std::endl;
        footer << "Version: " << PACKAGE_VERSION << "" << std::endl;
        footer << "----------------------------------------------------------------------------" << std::endl;
        footer << "Copyright (c) 2016-21 The Souffle Developers." << std::endl;
        footer << "Copyright (c) 2013-16 Oracle and/or its affiliates." << std::endl;
        footer << "All rights reserved." << std::endl;
        footer << "============================================================================" << std::endl;

        // command line options, the environment will be filled with the arguments passed to them, or
        // the empty string if they take none
        // main option, the datalog program itself, has an empty key
        std::vector<MainOption> options{{"", 0, "", "", false, ""},
                {"fact-dir", 'F', "DIR", ".", false, "Specify directory for fact files."},
                {"include-dir", 'I', "DIR", ".", true, "Specify directory for include files."},
                {"output-dir", 'D', "DIR", ".", false,
                        "Specify directory for output files. If <DIR> is `-` then stdout is used."},
                {"jobs", 'j', "N", "1", false,
                        "Run interpreter/compiler in parallel using N threads, N=auto for system "
                        "default."},
                {"compile", 'c', "", "", false,
                        "Generate C++ source code, compile to a binary executable, then run this "
                        "executable."},
                {"generate", 'g', "FILE", "", false,
                        "Generate C++ source code for the given Datalog program and write it to "
                        "<FILE>. If <FILE> is `-` then stdout is used."},
                {"inline-exclude", '\x7', "RELATIONS", "", false,
                        "Prevent the given relations from being inlined. Overrides any `inline` qualifiers."},
                {"swig", 's', "LANG", "", false,
                        "Generate SWIG interface for given language. The values <LANG> accepts is java and "
                        "python. "},
                {"library-dir", 'L', "DIR", "", true, "Specify directory for library files."},
                {"libraries", 'l', "FILE", "", true, "Specify libraries."},
                {"no-warn", 'w', "", "", false, "Disable warnings."},
                {"magic-transform", 'm', "RELATIONS", "", false,
                        "Enable magic set transformation changes on the given relations, use '*' "
                        "for all."},
                {"magic-transform-exclude", '\x8', "RELATIONS", "", false,
                        "Disable magic set transformation changes on the given relations. Overrides "
                        "`magic-transform`. Implies `inline-exclude` for the given relations."},
                {"macro", 'M', "MACROS", "", false, "Set macro definitions for the pre-processor"},
                {"disable-transformers", 'z', "TRANSFORMERS", "", false,
                        "Disable the given AST transformers."},
                {"dl-program", 'o', "FILE", "", false,
                        "Generate C++ source code, written to <FILE>, and compile this to a "
                        "binary executable (without executing it)."},
                {"live-profile", '\1', "", "", false, "Enable live profiling."},
                {"profile", 'p', "FILE", "", false, "Enable profiling, and write profile data to <FILE>."},
                {"profile-use", 'u', "FILE", "", false,
                        "Use profile log-file <FILE> for profile-guided optimization."},
                {"profile-frequency", '\2', "", "", false, "Enable the frequency counter in the profiler."},
                {"debug-report", 'r', "FILE", "", false, "Write HTML debug report to <FILE>."},
                {"pragma", 'P', "OPTIONS", "", false, "Set pragma options."},
                {"provenance", 't', "[ none | explain | explore ]", "", false,
                        "Enable provenance instrumentation and interaction."},
                {"verbose", 'v', "", "", false, "Verbose output."},
                {"version", '\3', "", "", false, "Version."},
                {"show", '\4',
                        "[ parse-errors | precedence-graph | scc-graph | transformed-datalog | "
                        "transformed-ram | type-analysis ]",
                        "", false, "Print selected program information."},
                {"parse-errors", '\5', "", "", false, "Show parsing errors, if any, then exit."},
                {"help", 'h', "", "", false, "Display this help message."},
                {"legacy", '\6', "", "", false, "Enable legacy support."}};
        Global::config().processArgs(argc, argv, header.str(), footer.str(), options);

        // ------ command line arguments -------------

        // Take in pragma options from the command line
        if (Global::config().has("pragma")) {
            std::vector<std::string> configOptions = splitString(Global::config().get("pragma"), ';');
            for (const std::string& option : configOptions) {
                std::size_t splitPoint = option.find(':');

                std::string optionName = option.substr(0, splitPoint);
                std::string optionValue = (splitPoint == std::string::npos)
                                                  ? ""
                                                  : option.substr(splitPoint + 1, option.length());

                if (!Global::config().has(optionName)) {
                    Global::config().set(optionName, optionValue);
                }
            }
        }

        /* for the version option, if given print the version text then exit */
        if (Global::config().has("version")) {
            std::cout << "Souffle: " << PACKAGE_VERSION;
            std::cout << "(" << RAM_DOMAIN_SIZE << "bit Domains)";
            std::cout << std::endl;
            std::cout << "Copyright (c) 2016-19 The Souffle Developers." << std::endl;
            std::cout << "Copyright (c) 2013-16 Oracle and/or its affiliates." << std::endl;
            return 0;
        }
        Global::config().set("version", PACKAGE_VERSION);

        /* for the help option, if given simply print the help text then exit */
        if (!Global::config().has("") || Global::config().has("help")) {
            std::cout << Global::config().help();
            return 0;
        }

        /* check that datalog program exists */
        if (!existFile(Global::config().get(""))) {
            throw std::runtime_error("cannot open file " + std::string(Global::config().get("")));
        }

        /* for the jobs option, to determine the number of threads used */
#ifdef _OPENMP
        if (isNumber(Global::config().get("jobs").c_str())) {
            if (std::stoi(Global::config().get("jobs")) < 1) {
                throw std::runtime_error("-j/--jobs may only be set to 'auto' or an integer greater than 0.");
            }
        } else {
            if (!Global::config().has("jobs", "auto")) {
                throw std::runtime_error("-j/--jobs may only be set to 'auto' or an integer greater than 0.");
            }
            // set jobs to zero to indicate the synthesiser and interpreter to use the system default.
            Global::config().set("jobs", "0");
        }
#else
        // Check that -j option has not been changed from the default
        if (Global::config().get("jobs") != "1" && !Global::config().has("no-warn")) {
            std::cerr << "\nThis installation of Souffle does not support concurrent jobs.\n";
        }
#endif

        /* if an output directory is given, check it exists */
        if (Global::config().has("output-dir") && !Global::config().has("output-dir", "-") &&
                !existDir(Global::config().get("output-dir")) &&
                !(Global::config().has("generate") ||
                        (Global::config().has("dl-program") && !Global::config().has("compile")))) {
            throw std::runtime_error(
                    "output directory " + Global::config().get("output-dir") + " does not exists");
        }

        /* collect all input directories for the c pre-processor */
        if (Global::config().has("include-dir")) {
            std::string currentInclude = "";
            std::string allIncludes = "";
            for (const char& ch : Global::config().get("include-dir")) {
                if (ch == ' ') {
                    if (!existDir(currentInclude)) {
                        throw std::runtime_error("include directory " + currentInclude + " does not exists");
                    } else {
                        allIncludes += " -I";
                        allIncludes += currentInclude;
                        currentInclude = "";
                    }
                } else {
                    currentInclude += ch;
                }
            }
            allIncludes += " -I" + currentInclude;
            Global::config().set("include-dir", allIncludes);
        }

        /* collect all macro definitions for the pre-processor */
        if (Global::config().has("macro")) {
            std::string currentMacro = "";
            std::string allMacros = "";
            for (const char& ch : Global::config().get("macro")) {
                if (ch == ' ') {
                    allMacros += " -D";
                    allMacros += currentMacro;
                    currentMacro = "";
                } else {
                    currentMacro += ch;
                }
            }
            allMacros += " -D" + currentMacro;
            Global::config().set("macro", allMacros);
        }

        /* turn on compilation of executables */
        if (Global::config().has("dl-program")) {
            Global::config().set("compile");
        }

        if (Global::config().has("live-profile") && !Global::config().has("profile")) {
            Global::config().set("profile");
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    /**
     * Ensure that code generation is enabled if using SWIG interface option.
     */
    if (Global::config().has("swig") && !Global::config().has("generate")) {
        Global::config().set("generate", simpleName(Global::config().get("")));
    }
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << "Current working directory: " << cwd << std::endl;
    } else {
        std::cerr << "Error getting current working directory" << std::endl;
    }
    if (!existDir("./souffle-analyze-data")) {
        std::filesystem::create_directory("./souffle-analyze-data");
        std::cout << "created dir" << std::endl;
    }
    // ------ start souffle -------------

    std::string souffleExecutable = which(argv[0]);

    if (souffleExecutable.empty()) {
        throw std::runtime_error("failed to determine souffle executable path");
    }

    /* Create the pipe to establish a communication between cpp and souffle */
    std::string cmd = which("mcpp");

    if (!isExecutable(cmd)) {
        throw std::runtime_error("failed to locate mcpp pre-processor");
    }

    cmd += " -e utf8 -W0 " + Global::config().get("include-dir");
    if (Global::config().has("macro")) {
        cmd += " " + Global::config().get("macro");
    }
    // Add RamDomain size as a macro
    cmd += " -DRAM_DOMAIN_SIZE=" + std::to_string(RAM_DOMAIN_SIZE);
    cmd += " " + Global::config().get("");
    FILE* in = popen(cmd.c_str(), "r");

    /* Time taking for parsing */
    auto parser_start = std::chrono::high_resolution_clock::now();

    // ------- parse program -------------

    // parse file
    ErrorReport errReport(Global::config().has("no-warn"));
    DebugReport debugReport;
    Own<ast::TranslationUnit> astTranslationUnit =
            ParserDriver::parseTranslationUnit("<stdin>", in, errReport, debugReport);

    // close input pipe
    int preprocessor_status = pclose(in);
    if (preprocessor_status == -1) {
        perror(nullptr);
        throw std::runtime_error("failed to close pre-processor pipe");
    }

    /* Report run-time of the parser if verbose flag is set */
    if (Global::config().has("verbose")) {
        auto parser_end = std::chrono::high_resolution_clock::now();
        std::cout << "Parse time: " << std::chrono::duration<double>(parser_end - parser_start).count()
                  << "sec\n";
    }

    if (Global::config().get("show") == "parse-errors") {
        std::cout << astTranslationUnit->getErrorReport();
        return astTranslationUnit->getErrorReport().getNumErrors();
    }

    // ------- check for parse errors -------------
    astTranslationUnit->getErrorReport().exitIfErrors();

    // ------- rewriting / optimizations -------------

    /* set up additional global options based on pragma declaratives */
    (mk<ast::transform::PragmaChecker>())->apply(*astTranslationUnit);

    /* construct the transformation pipeline */

    // Equivalence pipeline
    auto equivalencePipeline =
            mk<ast::transform::PipelineTransformer>(mk<ast::transform::NameUnnamedVariablesTransformer>(),
                    mk<ast::transform::FixpointTransformer>(mk<ast::transform::MinimiseProgramTransformer>()),
                    mk<ast::transform::ReplaceSingletonVariablesTransformer>(),
                    mk<ast::transform::RemoveRelationCopiesTransformer>(),
                    mk<ast::transform::RemoveEmptyRelationsTransformer>(),
                    mk<ast::transform::RemoveRedundantRelationsTransformer>());

    // Magic-Set pipeline
    auto magicPipeline = mk<ast::transform::PipelineTransformer>(mk<ast::transform::MagicSetTransformer>(),
            mk<ast::transform::ResolveAliasesTransformer>(),
            mk<ast::transform::RemoveRelationCopiesTransformer>(),
            mk<ast::transform::RemoveEmptyRelationsTransformer>(),
            mk<ast::transform::RemoveRedundantRelationsTransformer>(), clone(equivalencePipeline));

    // Partitioning pipeline
    auto partitionPipeline =
            mk<ast::transform::PipelineTransformer>(mk<ast::transform::NameUnnamedVariablesTransformer>(),
                    mk<ast::transform::PartitionBodyLiteralsTransformer>(),
                    mk<ast::transform::ReplaceSingletonVariablesTransformer>());

    // Provenance pipeline
    auto provenancePipeline = mk<ast::transform::ConditionalTransformer>(Global::config().has("provenance"),
            mk<ast::transform::PipelineTransformer>(mk<ast::transform::ExpandEqrelsTransformer>(),
                    mk<ast::transform::NameUnnamedVariablesTransformer>()));

    // Main pipeline
    auto pipeline = mk<ast::transform::PipelineTransformer>(mk<ast::transform::ComponentChecker>(),
            mk<ast::transform::ComponentInstantiationTransformer>(),
            mk<ast::transform::IODefaultsTransformer>(),
            mk<ast::transform::SimplifyAggregateTargetExpressionTransformer>(),
            mk<ast::transform::UniqueAggregationVariablesTransformer>(),
            mk<ast::transform::FixpointTransformer>(mk<ast::transform::PipelineTransformer>(
                    mk<ast::transform::ResolveAnonymousRecordAliasesTransformer>(),
                    mk<ast::transform::FoldAnonymousRecords>())),
            mk<ast::transform::SemanticChecker>(), mk<ast::transform::GroundWitnessesTransformer>(),
            mk<ast::transform::UniqueAggregationVariablesTransformer>(),
            mk<ast::transform::MaterializeSingletonAggregationTransformer>(),
            mk<ast::transform::FixpointTransformer>(
                    mk<ast::transform::MaterializeAggregationQueriesTransformer>()),
            mk<ast::transform::RemoveRedundantSumsTransformer>(),
            mk<ast::transform::NormaliseGeneratorsTransformer>(),
            mk<ast::transform::ResolveAliasesTransformer>(),
            mk<ast::transform::RemoveBooleanConstraintsTransformer>(),
            mk<ast::transform::ResolveAliasesTransformer>(), mk<ast::transform::MinimiseProgramTransformer>(),
            mk<ast::transform::InlineUnmarkExcludedTransform>(),
            mk<ast::transform::InlineRelationsTransformer>(), mk<ast::transform::GroundedTermsChecker>(),
            mk<ast::transform::ResolveAliasesTransformer>(),
            mk<ast::transform::RemoveRedundantRelationsTransformer>(),
            mk<ast::transform::RemoveRelationCopiesTransformer>(),
            mk<ast::transform::RemoveEmptyRelationsTransformer>(),
            mk<ast::transform::ReplaceSingletonVariablesTransformer>(),
            mk<ast::transform::FixpointTransformer>(mk<ast::transform::PipelineTransformer>(
                    mk<ast::transform::ReduceExistentialsTransformer>(),
                    mk<ast::transform::RemoveRedundantRelationsTransformer>())),
            mk<ast::transform::RemoveRelationCopiesTransformer>(), std::move(partitionPipeline),
            std::move(equivalencePipeline), mk<ast::transform::RemoveRelationCopiesTransformer>(),
            std::move(magicPipeline), mk<ast::transform::ReorderLiteralsTransformer>(),
            mk<ast::transform::RemoveEmptyRelationsTransformer>(),
            mk<ast::transform::AddNullariesToAtomlessAggregatesTransformer>(),
            mk<ast::transform::ReorderLiteralsTransformer>(), mk<ast::transform::ExecutionPlanChecker>(),
            std::move(provenancePipeline), mk<ast::transform::IOAttributesTransformer>());

    // Disable unwanted transformations
    if (Global::config().has("disable-transformers")) {
        std::vector<std::string> givenTransformers =
                splitString(Global::config().get("disable-transformers"), ',');
        pipeline->disableTransformers(
                std::set<std::string>(givenTransformers.begin(), givenTransformers.end()));
    }

    // Set up the debug report if necessary
    if (Global::config().has("debug-report")) {
        auto parser_end = std::chrono::high_resolution_clock::now();
        std::stringstream ss;

        // Add current time
        std::time_t time = std::time(nullptr);
        ss << "Executed at ";
        ss << std::put_time(std::localtime(&time), "%F %T") << "\n";

        // Add config
        ss << "(\n";
        ss << join(Global::config().data(), ",\n", [](std::ostream& out, const auto& arg) {
            out << "  \"" << arg.first << "\" -> \"" << arg.second << '"';
        });
        ss << "\n)";

        debugReport.addSection("Configuration", "Configuration", ss.str());

        // Add parsing runtime
        std::string runtimeStr =
                "(" + std::to_string(std::chrono::duration<double>(parser_end - parser_start).count()) + "s)";
        debugReport.addSection("Parsing", "Parsing " + runtimeStr, "");

        pipeline->setDebugReport();
    }

    // Toggle pipeline verbosity
    pipeline->setVerbosity(Global::config().has("verbose"));

    // Apply all the transformations
    pipeline->apply(*astTranslationUnit);

    if (Global::config().has("show")) {
        // Output the transformed datalog and return
        if (Global::config().get("show") == "transformed-datalog") {
            std::cout << astTranslationUnit->getProgram() << std::endl;
            return 0;
        }

        // Output the precedence graph in graphviz dot format and return
        if (Global::config().get("show") == "precedence-graph") {
            astTranslationUnit->getAnalysis<ast::analysis::PrecedenceGraphAnalysis>()->print(std::cout);
            std::cout << std::endl;
            return 0;
        }

        // Output the scc graph in graphviz dot format and return
        if (Global::config().get("show") == "scc-graph") {
            astTranslationUnit->getAnalysis<ast::analysis::SCCGraphAnalysis>()->print(std::cout);
            std::cout << std::endl;
            return 0;
        }

        // Output the type analysis
        if (Global::config().get("show") == "type-analysis") {
            astTranslationUnit->getAnalysis<ast::analysis::TypeAnalysis>()->print(std::cout);
            std::cout << std::endl;
            return 0;
        }
    }

    // ------- execution -------------
    /* translate AST to RAM */
    debugReport.startSection();
    auto translationStrategy =
            Global::config().has("provenance")
                    ? mk<ast2ram::TranslationStrategy, ast2ram::provenance::TranslationStrategy>()
                    : mk<ast2ram::TranslationStrategy, ast2ram::seminaive::TranslationStrategy>();
    auto unitTranslator = Own<ast2ram::UnitTranslator>(translationStrategy->createUnitTranslator());
    auto ramTranslationUnit = unitTranslator->translateUnit(*astTranslationUnit);
    debugReport.endSection("ast-to-ram", "Translate AST to RAM");

    // Apply RAM transforms
    {
        using namespace ram::transform;
        Own<Transformer> ramTransform = mk<TransformerSequence>(
                mk<LoopTransformer>(mk<TransformerSequence>(mk<ExpandFilterTransformer>(),
                        mk<HoistConditionsTransformer>(), mk<MakeIndexTransformer>())),
                mk<IfConversionTransformer>(), mk<IfExistsConversionTransformer>(),
                mk<CollapseFiltersTransformer>(), mk<TupleIdTransformer>(),
                mk<LoopTransformer>(
                        mk<TransformerSequence>(mk<HoistAggregateTransformer>(), mk<TupleIdTransformer>())),
                mk<ExpandFilterTransformer>(), mk<HoistConditionsTransformer>(),
                mk<CollapseFiltersTransformer>(), mk<EliminateDuplicatesTransformer>(),
                mk<ReorderConditionsTransformer>(), mk<LoopTransformer>(mk<ReorderFilterBreak>()),
                mk<ConditionalTransformer>(
                        // job count of 0 means all cores are used.
                        []() -> bool { return std::stoi(Global::config().get("jobs")) != 1; },
                        mk<ParallelTransformer>()),
                mk<ReportIndexTransformer>());

        ramTransform->apply(*ramTranslationUnit);
    }

    if (ramTranslationUnit->getErrorReport().getNumIssues() != 0) {
        std::cerr << ramTranslationUnit->getErrorReport();
    }
    std::cout << ramTranslationUnit->getProgram();
    // Output the transformed RAM program and return
    if (Global::config().get("show") == "transformed-ram") {
        std::cout << ramTranslationUnit->getProgram();
        return 0;
    }

    try {
        if (!Global::config().has("compile") && !Global::config().has("dl-program") &&
                !Global::config().has("generate") && !Global::config().has("swig")) {
            // ------- interpreter -------------

            std::thread profiler;
            // Start up profiler if needed
            if (Global::config().has("live-profile") && !Global::config().has("compile")) {
                profiler = std::thread([]() { profile::Tui().runProf(); });
            }

            // configure and execute interpreter
            Own<interpreter::Engine> interpreter(mk<interpreter::Engine>(*ramTranslationUnit,
                    "./souffle-analyze-data/output_" + std::to_string(modified_souffle::countFilesInDirectory(
                                                               "./souffle-analyze-data/"))));
            interpreter->executeMain();
            // If the profiler was started, join back here once it exits.
            if (profiler.joinable()) {
                profiler.join();
            }
            if (Global::config().has("provenance")) {
                // only run explain interface if interpreted
                interpreter::ProgInterface interface(*interpreter);
                if (Global::config().get("provenance") == "explain") {
                    explain(interface, false);
                } else if (Global::config().get("provenance") == "explore") {
                    explain(interface, true);
                }
            }
        } else {
            // ------- compiler -------------
            // int jobs = std::stoi(Global::config().get("jobs"));
            // jobs = (jobs <= 0 ? MAX_THREADS : jobs);
            auto synthesiser =
                    mk<synthesiser::Synthesiser>(/*static_cast<std::size_t>(jobs),*/ *ramTranslationUnit);

            // Find the base filename for code generation and execution
            std::string baseFilename;
            if (Global::config().has("dl-program")) {
                baseFilename = Global::config().get("dl-program");
            } else if (Global::config().has("generate")) {
                baseFilename = Global::config().get("generate");

                // trim .cpp extension if it exists
                if (baseFilename.size() >= 4 && baseFilename.substr(baseFilename.size() - 4) == ".cpp") {
                    baseFilename = baseFilename.substr(0, baseFilename.size() - 4);
                }
            } else {
                baseFilename = tempFile();
            }
            if (baseName(baseFilename) == "/" || baseName(baseFilename) == ".") {
                baseFilename = tempFile();
            }

            std::string baseIdentifier = identifier(simpleName(baseFilename));
            std::string sourceFilename = baseFilename + ".cpp";

            bool withSharedLibrary;
            auto synthesisStart = std::chrono::high_resolution_clock::now();
            const bool emitToStdOut = Global::config().has("generate", "-");
            if (emitToStdOut)
                synthesiser->generateCode(std::cout, baseIdentifier, withSharedLibrary);
            else {
                std::ofstream os{sourceFilename};
                synthesiser->generateCode(os, baseIdentifier, withSharedLibrary);
            }
            if (Global::config().has("verbose")) {
                auto synthesisEnd = std::chrono::high_resolution_clock::now();
                std::cout << "Synthesis time: "
                          << std::chrono::duration<double>(synthesisEnd - synthesisStart).count() << "sec\n";
            }

            if (withSharedLibrary) {
                if (!Global::config().has("libraries")) {
                    Global::config().set("libraries", "functors");
                }
                if (!Global::config().has("library-dir")) {
                    Global::config().set("library-dir", ".");
                }
            }

            auto findCompileCmd = [&] {
                auto cmd = findTool("souffle-compile", souffleExecutable, ".");
                /* Fail if a souffle-compile executable is not found */
                if (!isExecutable(cmd)) {
                    throw std::runtime_error("failed to locate souffle-compile");
                }
                return cmd;
            };

            auto compileStart = std::chrono::high_resolution_clock::now();
            if (Global::config().has("swig")) {
                auto compileCmd = findCompileCmd() + " -s " + Global::config().get("swig") + " ";
                compileToBinary(compileCmd, sourceFilename);
            } else if (Global::config().has("compile")) {
                compileToBinary(findCompileCmd(), sourceFilename);
                /* Report overall run-time in verbose mode */
                // run compiled C++ program if requested.
                if (!Global::config().has("dl-program") && !Global::config().has("swig")) {
                    executeBinary(baseFilename);
                }
            }
            if (Global::config().has("verbose")) {
                auto compileEnd = std::chrono::high_resolution_clock::now();
                std::cout << "Compilation time: "
                          << std::chrono::duration<double>(compileEnd - compileStart).count() << "sec\n";
            }
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* Report overall run-time in verbose mode */
    if (Global::config().has("verbose")) {
        auto souffle_end = std::chrono::high_resolution_clock::now();
        std::cout << "Total time: " << std::chrono::duration<double>(souffle_end - souffle_start).count()
                  << "sec\n";
    }

    return 0;
}

}  // end of namespace souffle

int main(int argc, char** argv) {
    return souffle::main(argc, argv);
}
