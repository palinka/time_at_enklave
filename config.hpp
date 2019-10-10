#ifndef TIME_AT_ENKLAVE_CONFIG_HPP
#define TIME_AT_ENKLAVE_CONFIG_HPP

#include <string>
#include <regex>

namespace enklave {
    /// Values used in more than two places (e.g. main and tests/) are defined here.
    namespace config {
        constexpr char path_with_mails[] = "../tests/data/";
    }
}

#endif //TIME_AT_ENKLAVE_CONFIG_HPP
