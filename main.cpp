#include <iostream>
#include "enklave.hpp"

int main(int argc, char* argv[]) {
    using namespace enklave;

    // TODO Use a smarter init.
    string path_with_mails{enklave::config::path_with_mails};
    if(argc > 1) // If path is passed in by first argument, override configured path.
        path_with_mails = argv[1];

    auto results = scan_directory(enklave::config::path_with_mails);

    // Inform user if no files were found.
    /*
    if(files_paths.empty()) {
        std::cerr << "No relevant files found in folder: " << path_with_mails << '\n';
        return 1;
    }
    */

    //auto result = compute_timeslots(parse(files_paths));
    //std::cout << "Time spent at enklave: " << date::format("%T", result) << '\n';

    return 0;
}