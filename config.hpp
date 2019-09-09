#ifndef TIME_AT_ENKLAVE_CONFIG_HPP
#define TIME_AT_ENKLAVE_CONFIG_HPP

#include <string>
#include <regex>

//File containing namespace for configuration
namespace enklave {
    namespace config {
        const std::string folder_with_mails{"../tests/data/"};
        const std::regex relevant_mails_filename_contains{"Confirmation_"};
        const std::regex check_in_pattern{"Check in"};
        const std::regex check_out_pattern{"Check out"};
    }
}

#endif //TIME_AT_ENKLAVE_CONFIG_HPP
