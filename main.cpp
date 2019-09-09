#include <iostream>
#include "enklave.hpp"

int main() {
    using namespace date;
    using namespace enklave;

    auto files_paths = get_relevant_file_paths_from_folder(enklave::config::folder_with_mails);
    auto slots = parse(files_paths);
    auto result = compute_duration(slots);
    std::cout << "Time spent at enklave: " << format("%T", result) << '\n';

    return 0;
}