#include "doctest.h"
#include "GameTest.cpp"
#include "AnalyzerTest.cpp"
#include "ShapeTest.cpp"
#include "MCTSTest.cpp"

int startTest(int argc, char *argv[]) {
    // 运行 doctest 测试
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int test_result = context.run();

    // 根据测试结果返回退出码
    if (test_result != 0) {
        std::cerr << "Tests failed!" << std::endl;
        return test_result;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}