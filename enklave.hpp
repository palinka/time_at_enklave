#ifndef TIME_AT_ENKLAVE_ENKLAVE_HPP
#define TIME_AT_ENKLAVE_ENKLAVE_HPP

#include <fstream>
#include <istream>
#include <regex>
#include <chrono>
#include <set>
#include <optional>
#include <algorithm>
#include <numeric>
#include <fstream>

#include "include/date.h"
#include "config.hpp"

// Filesystem needs some care on different compilers.
#include <filesystem>
#ifdef _WIN32
namespace fs = std::experimental::filesystem::v1;
#elif __linux__
namespace fs = std::filesystem;
#endif

namespace enklave {
    using namespace std;

    // TODO use inheritance?

    // Default value for sys_seconds is '0'.
    // Using the typesystem (inheritance, optionals and/or variants) to have clearer
    // types indicating "no value" resulted in overblown code. So, declare "zero" in namespace scope to indicate
    // "no value" clearly.
    constexpr date::sys_seconds zero;

    // TODO Add relevant keywords.
    struct check_in_or_out {
        date::sys_seconds in;
        date::sys_seconds out;

        date::sys_seconds get_value() {
            if (in != zero)
                return in;
            if (out != zero)
                return out;
            return zero;
        }
    };

    // From check_in_or_out's timeslots are computed.
    using timeslot = pair<date::sys_seconds, date::sys_seconds>;
    using duration = chrono::duration<int>; // Let's use int for durations.

/** TODO Update doc, timezone will be ingored.
 *
 * Converts a string containing a ISO 8601-like date to the date::sys_seconds.
 *
 * ISO 8601-like strings that use "_" instead of ":" are valid input, e.g. "Confirmation 2019-08-22T15_31_05+02_00.eml".
 *
 * Returns an optional to avoid any confusions with '0' which is a valid value for date::sys_seconds.
 *
 * @param str String in ISO 8601-like format.
 * @return std::optional<date::sys_seconds> in UTC.
 */
    std::optional<date::sys_seconds> parse_datetime(const string &input) noexcept {
        using namespace date;

        // Reduce input string to datetime.
        smatch matches_date_time; // Contains result.
        const regex date_time_pattern{R"(\d{2}\s\w{3}\s\d{4}\s\d{2}:\d{2}:\d{2}\s\+\d{4})"};
        regex_search(input, matches_date_time, date_time_pattern);

        // Stream datetime string to date::parse.
        stringstream extracted_date{matches_date_time[0]};
        date::sys_seconds point_in_time;
        // Note: timezone is stripped away for now.
        extracted_date >> date::parse("%d %b %Y %T", point_in_time);

        if (bool(extracted_date)) {
            return point_in_time;
        } else {
            return nullopt;
        }
    }

    // TODO https://www.walletfox.com/course/patternmatchingcpp17.php
    // Assume all files have the same structure, so parse top to bottom.
    check_in_or_out parse_file(const fs::path &f) noexcept(false) {
        const regex is_from_enklave_regex{"header.from=enklave.de"};
        const regex date_regex("X-Pm-Date:");
        const regex check_in_regex("Check_in");

        check_in_or_out result;
        ifstream ifs{f};
        string line;
        bool isCheckIn = false;

        // First line must in file must contain is_from_enklave_regex
        if (!getline(ifs, line))
            ifs.exceptions(ifstream::failbit); // Throw if stream fails, e.g. file not exists or is empty.

        // TODO an empty check_in_or_out could also be returned.
        if (!regex_search(line, is_from_enklave_regex)) {
            throw runtime_error{"Parsed file is not an email from enklave: " + f.string()};
        }

        while (getline(ifs, line)) {
            // Is it a check in?
            if (regex_search(line, check_in_regex)) {
                cout << "Found a check-in: " << line << '\n';
                isCheckIn = true;
            }

            // TODO Catch case where it is not a check-in nor a check-out etc.?

            if (regex_search(line, date_regex)) {
                auto datetime = parse_datetime(line);
                if (!datetime)
                    cerr << "Failed to parse datetime of file: " << f << '\n';
                if (isCheckIn) {
                    result.in = datetime.value();
                } else {
                    result.out = datetime.value();
                }
            }
        }
        return result;
    }

    vector<check_in_or_out> scan_directory(const fs::path &p) {
        cout << "Scanning for relevant files in: " << p << ":\n";
        vector<check_in_or_out> enklave_events;
        check_in_or_out ee;
        for (const fs::directory_entry &x: fs::directory_iterator(p)) {
            const fs::path &f = x;
            // TODO Use a std::iterator and a predicate function.
            if (f.extension() == ".eml") {
                std::cout << "Parsing file: " << f << '\n';
                try {
                    ee = parse_file(f);
                    enklave_events.push_back(ee);
                } catch (runtime_error e) {
                    cerr << e.what(); // e.g. file could be opened, but parsing did not meet criteria.
                } // Let caller catch all other exceptions, e.g. from filesystem.
            }
        }
        return enklave_events;
    }

    // Computing timeslots actually wastes space, because vector<check_in_or_outs> could be directly used.
    // However, this intermediate steps appears more maintainable to me.
    vector<timeslot> compute_timeslots(vector<check_in_or_out> &all_in_or_outs) noexcept {
        vector<timeslot> result;

        // Sort by time.
        // TODO make it an operator.
        auto sorting_function = [](check_in_or_out &c1, check_in_or_out &c2) {
            return c1.get_value() < c2.get_value();
        };

        sort(all_in_or_outs.begin(), all_in_or_outs.end(), sorting_function);

        // TODO Find adjacent in/out and remove them.

        for (auto current = all_in_or_outs.begin(); current != all_in_or_outs.end(); ++current) {
            result.push_back(timeslot{current->get_value(), next(current)->get_value()});
            ++current;
        }

        return result;
    }

    duration compute_duration(vector<timeslot> slots) {
        return std::accumulate(slots.begin(), slots.end(), 0s, [](duration accumulator, auto slot) {
            return accumulator + (slot.second - slot.first);
        });

    }
}
#endif //TIME_AT_ENKLAVE_ENKLAVE_HPP
