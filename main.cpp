#include <iostream>
#include "enklave.hpp"

int main(int argc, char *argv[]) {
    using namespace enklave;

    string path_with_mails{enklave::config::path_with_mails};

    if (argc > 1) // If path is passed in by first argument, override configured path.
        path_with_mails = argv[1];

    auto found_events = scan_directory(path_with_mails);

    // Provide some user feedback:
    std::cout << "The following events were found:" << std::endl;
    for (auto &x : found_events) {
        std::cout << x;
    }

    if (found_events.size() < 2) {
        std::cerr << "Scanned directory does not contain files with at least one check-in and one check-out."
                  << std::endl;
        return 0;
    }

    auto timeslots = compute_timeslots(found_events);
    auto result = compute_duration(timeslots);

    std::cout << "Time spent at enklave: " << date::format("%T", result) << '\n';
    return 0;
}