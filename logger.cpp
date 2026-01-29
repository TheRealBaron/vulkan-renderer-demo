#include <iostream>
#include "logger.hpp"


void logger::log(const std::string& s) {

#ifndef NDEBUG
    std::cout << "[DEBUG] ";
    std::cout << s << std::endl;
#endif

}
