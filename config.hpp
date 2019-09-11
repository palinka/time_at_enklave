#ifndef TIME_AT_ENKLAVE_CONFIG_HPP
#define TIME_AT_ENKLAVE_CONFIG_HPP

#include <string>
#include <regex>

//File containing namespace for configuration
// TODO get rid of global vars / use contexpr.
namespace enklave {
    namespace config {
        const std::string path_with_mails{"../tests/data/"};
        const std::regex relevant_files_regex{"Confirmation_"};
        const std::regex check_in_pattern{"Check in"};
        const std::regex check_out_pattern{"Check out"};
    }
}

#endif //TIME_AT_ENKLAVE_CONFIG_HPP
