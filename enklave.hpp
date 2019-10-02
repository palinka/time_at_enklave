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
#include <variant>

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

    // Let's use int for durations.
    using duration = chrono::duration<int>;

    // Timeslots consist in a pair of a check-in and a check-out.
    // TODO use proper types instead of sys_seconds.
    using timeslot = pair<date::sys_seconds, date::sys_seconds>;

    struct check_in_or_out {
        date::sys_seconds when;
        fs::path file;
    };

    struct check_in : check_in_or_out {
    };

    struct check_out : check_in_or_out {
    };

    // Instead of holding one member variable for check-in and one for check-out in a class , a variant can be used.
    // This also eliminates determining which event an instance holds via checking against values, e.g. considering
    // date::sys_seconds zero(std::chrono::seconds(0)) being "this member is not used/initialized", so it must be
    // the other member.
    class enklave_event {
    private:
        variant<check_in, check_out> m_check_in_or_out;

    public:
        date::sys_seconds get_when() const {
            return std::visit([](auto &&event) { return event.when; }, m_check_in_or_out);
        }

        fs::path get_filepath() const {
            return std::visit([](auto &&event) { return event.file; }, m_check_in_or_out);
        }

        // Using operator= for implicit conversations via assignment.
        enklave_event &operator=(check_in event) {
            m_check_in_or_out = event;
            return *this;
        }

        enklave_event &operator=(check_out event) {
            m_check_in_or_out = event;
            return *this;
        }

        friend std::ostream &operator<<(std::ostream &out, const enklave_event &event);

        bool is_check_in() const {
            return std::holds_alternative<check_in>(m_check_in_or_out);
        };
    };

    std::ostream &operator<<(std::ostream &out, const enklave_event &event) {
        if (event.is_check_in()) {
            out << "Check-in  at: " << date::format("%F %T", event.get_when()) << "\n";
        } else {
            out << "Check-out at: " << date::format("%F %T", event.get_when()) << "\n";
        }
        out << "File: " << event.get_filepath().string() << '\n';
        return out;
    }


    /** Converts a string containing a ISO 8601-like date to date::sys_seconds.
    *
    * ISO 8601-like strings that use "_" instead of ":" are valid input, e.g. "Confirmation 2019-08-22T15_31_05+02_00.eml".
    *
    * Returns an optional to avoid any confusions with '0' which is a valid value for date::sys_seconds.
    *
    * Note: timezones will be stripped away.
    *
    * @param input String in ISO 8601-like format.
    * @return std::optional<date::sys_seconds>.
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

    /** Parse a file from top to bottom.
     *
     * The returned object contains the information if it was a check-in or a check-out and when it happened.
     * It function can throw runtime_errors for various reasons.
     *
     * @param f Path to a file
     * @return check_in_or_out. TODO link to type doc.
     */

    enklave_event parse_file(const fs::path &f) noexcept(false) {
        std::cout << "Parsing file: " << f << '\n';
        const regex is_from_enklave_regex{"header.from=enklave.de"};
        const regex date_regex("X-Pm-Date:");
        const regex check_in_regex("Check_in");
        const regex check_out_regex("Check out");

        enklave_event result;
        ifstream ifs{f};
        string line;
        bool isCheckIn = false;
        bool isCheckOut = false;

        // First line must in file must contain is_from_enklave_regex
        if (!getline(ifs, line)) {
            throw runtime_error{"Could not open file or get the first line: " + f.string()};
        }

        if (!regex_search(line, is_from_enklave_regex)) {
            throw runtime_error{"Parsed file is not an email from enklave: " + f.string()};
        }

        /* After the first line was parsed, read the rest of the file from top to bottom and assume:
         * - First check_in_regex OR check_out_regex appears in file determining with event it was.
         * - In the lines afterwards the datetime of the event is found.
         */
        while (getline(ifs, line)) {
            // Does the actual line identify a check-in?
            if (regex_search(line, check_in_regex))
                isCheckIn = true;

            // Does the actual line identify a check-out?
            if (regex_search(line, check_out_regex))
                isCheckOut = true;

            // If a valid datetime pattern is found.
            if (regex_search(line, date_regex)) {
                if (!isCheckIn && !isCheckOut) {
                    throw runtime_error{"Parsed file is neither a check-in nor a check-out: " + f.string()};
                } // Assume no file that is a check-in AND a check-out exists.

                // Parse datetime.
                auto datetime = parse_datetime(line);
                if (!datetime) {
                    throw runtime_error{"Datetime could not be parsed: " + f.string()};
                }

                if (isCheckIn)
                    result = check_in{date::sys_seconds{datetime.value()}, f};
                if (isCheckOut)
                    result = check_out{date::sys_seconds{datetime.value()}, f};
            }
        }
        return result;
    }

    /** Scans all files in a directory and returns a vector with parsed check_in_or_out.
     *
     * @param p Path do a directory.
     * @return Vector with all check_in_or_out's.
     */
    vector<enklave_event> scan_directory(const fs::path &p) {
        cout << "Scanning for relevant files in: " << p << ":\n";
        vector<enklave_event> enklave_events;
        enklave_event ee;
        for (const fs::directory_entry &x: fs::directory_iterator(p)) {
            const fs::path &f = x;
            // TODO Use a std::iterator and a predicate function.
            if (f.extension() == ".eml") {
                try {
                    ee = parse_file(f);
                    enklave_events.push_back(ee);
                } catch (runtime_error &e) {
                    cerr << e.what() << '\n'; // e.g. file could be opened, but parsing did not meet criteria.
                } // Let caller catch all other exceptions, e.g. from filesystem.
            }
        }
        return enklave_events;
    }

    // Computing timeslots actually wastes space, because vector<check_in_or_outs> could be directly used.
    // However, this intermediate steps appears more maintainable to me.
    vector<timeslot> compute_timeslots(vector<enklave_event> &all_in_or_outs) noexcept(false) {
        vector<timeslot> result;

        if (all_in_or_outs.size() < 2) {
            throw logic_error("At least 2 events must be provided.");
        }

        // Sort by time.
        // TODO make it an operator.
        auto sorting_function = [](check_in_or_out &c1, check_in_or_out &c2) {
            return c1.get_value() < c2.get_value();
        };

        sort(all_in_or_outs.begin(), all_in_or_outs.end(), sorting_function);


        /* TODO impossible events
        auto impossible_events_predicate = [](check_in_or_out first, check_in_or_out second) {
            if ((first.is_check_in() && second.is_check_in()) || (!first.is_check_in() && !second.is_check_in())) {
                cerr << "removing ONE: ";
                first.print_value();
                return false;
            } else {
                return true;
            }
        };
        // TODO Find adjacent in/out and remove them.
        unique(all_in_or_outs.begin(), all_in_or_outs.end(), impossible_events_predicate);
        */

        if (all_in_or_outs.size() % 2 != 0) {
            throw logic_error("Check-ins and check-outs are pairs. The provided dataset does not have an even size.");
        }
        /* Copy elements from vector<check_in_or_out> to vector<pair<date::sys_seconds, date::sys_seconds>>
         * where the first element of the pair represents a check-in and the second a check-out.
         * I am not aware of an std::algorithm iterating a container in x[n] / x[n+1] fashion.
         * Above check ensures the manual loop will always terminate.
         */
        for (auto check_in = all_in_or_outs.begin(); check_in != all_in_or_outs.end(); ++check_in) {
            auto check_out = next(check_in);
            result.push_back(timeslot{check_in->get_when(), check_out->get_when()});
            ++check_in;
        }
        // TODO Check for size > 1;

        /*
        auto check_in = all_in_or_outs.begin();
        auto check_out = next(check_in);
        while (check_in != all_in_or_outs.end()) {
            if (check_in->is_check_in() && check_out->is_check_in()) {
                cerr << "Removed one!" << '\n';
            } else {
                check_in->print_value();
                result.emplace_back(timeslot{check_in->get_value(), check_out->when()});
            }
            check_in = next(check_in, 2);
        }
        */

        if (all_in_or_outs.size() / 2 != result.size()) {
            throw logic_error("The computed timeslots must be half of the size of all check-ins and check-outs.");
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
