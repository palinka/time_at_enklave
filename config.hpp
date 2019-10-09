#ifndef TIME_AT_ENKLAVE_CONFIG_HPP
#define TIME_AT_ENKLAVE_CONFIG_HPP

#include <string>
#include <regex>

/** File containing namespace for configuration.
 * Values used in more than two places (e.g. main and tests/) are defined here.
 **/
namespace enklave {
    namespace config {
        const static std::string path_with_mails{"../tests/data/"};
    }
}

#endif //TIME_AT_ENKLAVE_CONFIG_HPP
