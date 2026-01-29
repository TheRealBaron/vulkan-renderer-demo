#include <iostream>
#include "myapp.h"


int main(int argc, const char* argv[]) {
    try {
        myapp::run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
