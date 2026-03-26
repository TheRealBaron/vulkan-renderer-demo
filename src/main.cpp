#include "myapp.h"
#include "utils/logger.hpp"


int main(int argc, const char* argv[]) {
    try {
        myapp::run();
    } catch (const std::exception& e) {
        logger::log(LStatus::FATAL, "caught runtime error: {}", e.what());
        return -1;
    }

    return 0;
}
