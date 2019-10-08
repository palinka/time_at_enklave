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
            return visit([](auto &&event) { return event.when; }, m_check_in_or_out);
        }

        fs::path get_filepath() const {
            return visit([](auto &&event) { return event.file; }, m_check_in_or_out);
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

        bool is_check_in() const {
            return holds_alternative<check_in>(m_check_in_or_out);
        };
    };

    ostream &operator<<(ostream &out, const enklave_event &event) {
        if (event.is_check_in()) {
            out << "Check-in  at: " << date::format("%F %T", event.get_when()) << endl;
        } else {
            out << "Check-out at: " << date::format("%F %T", event.get_when()) << endl;
        }
        out << "File: " << event.get_filepath().string() << endl;
        return out;
    }

    // Timeslots consist in a pair of a check-in and a check-out contained in type enklave_event.
    using timeslot = pair<enklave_event &, enklave_event &>;

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
    optional<date::sys_seconds> parse_datetime(const string &input) noexcept {
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
     * This function can throw runtime_errors for various reasons.
     *
     * @param f Path to a file
     * @return enklave_event. TODO link to type doc.
     */

    enklave_event parse_file(const fs::path &f) noexcept(false) {
        //cout << "Parsing file: " << f << endl;
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

    /** Scans all files in a directory and returns a vector with parsed data.
     *
     * @param p Path do a directory.
     * @return Vector with all enklave_event's. TODO reference type.
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
                    cerr << e.what() << endl; // e.g. file could be opened, but parsing did not meet criteria.
                } // Let the caller catch all other exceptions, e.g. from filesystem.
            }
        }
        return enklave_events;
    }

    vector<timeslot> compute_timeslots(vector<enklave_event> &events) noexcept(false) {
        vector<timeslot> result;

        if (events.size() < 2) {
            throw logic_error("At least 2 events must be provided.");
        }

        // Sort by time.
        // TODO make it an operator.
        auto sorting_function = [](enklave_event &c1, enklave_event &c2) {
            return c1.get_when() < c2.get_when();
        };

        sort(events.begin(), events.end(), sorting_function);

        auto impossible_event_predicate = [](const enklave_event &first, const enklave_event &second) {
            // If both events are of same type.
            return (first.is_check_in() && second.is_check_in()) || (!first.is_check_in() && !second.is_check_in());
        };

        // Deal with forgotten check-in's or check-out's. Such missing events result in adjacent events of the same
        // type in the time-sorted vector.
        // This do-while loop will keep only the last (oldest) event and delete younger adjacent ones.
        auto it = events.begin();
        do {
            it = adjacent_find(it, events.end(), impossible_event_predicate);
            if (it != events.end()) {
                cerr << "The following event is impossible and thus removed from computation:" << endl;
                cerr << *it;
                it = events.erase(it);
            }

        } while (it != events.end());

        if (events.size() % 2 != 0) {
            cerr << "The last event in the list is removed, it misses its check-out." << endl;
            cerr << events.back();
            events.pop_back();
        }

        /* Arrange vector holding single events in pairs such that each pair represents a timeslot.
         * The first element of the pair represents a check-in and the second the corresponding check-out.
         *
         * Above check ensures the manual loop will always terminate.
         *
         * Note: this loop iterates on container type enklave_event.
         */
        for (auto check_in = events.begin(); check_in != events.end(); ++check_in) {
            auto check_out = next(check_in);
            result.push_back(make_pair(ref(*check_in), ref(*check_out)));
            // Most probably this would be more readable:
            // result.push_back(timeslot{*check_in, *check_out});
            ++check_in;
        }

        // Do some sanity checks of result.
        // TODO Add rounding of size is not even.
        if (events.size() / 2 != result.size()) {
            throw logic_error("The computed timeslots must be half of the size of all check-ins and check-outs.");
        }

        for (auto &x: result) {
            if (!x.first.is_check_in() || x.second.is_check_in()) {
                throw logic_error("An unexpected logic error occurred: one or more check-ins and/or check-outs are "
                                  "interchanged.");
            }
        }
        return result;
    }

    duration compute_duration(const vector<timeslot> &slots) {
        return accumulate(slots.begin(), slots.end(), 0s, [](duration accumulator, timeslot slot) {
            return accumulator + (slot.second.get_when() - slot.first.get_when());
        });
    }
}
#endif //TIME_AT_ENKLAVE_ENKLAVE_HPP
