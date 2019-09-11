#include <iostream>
#include "enklave.hpp"

int main() {
    using namespace enklave;

    auto files_paths = get_relevant_files(enklave::config::path_with_mails, enklave::config::relevant_files_regex);
    auto result = compute_duration(parse(files_paths));
    std::cout << "Time spent at enklave: " << date::format("%T", result) << '\n';

    return 0;
}