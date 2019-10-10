#ifndef TIME_AT_ENKLAVE_ENKLAVE_HPP
#define TIME_AT_ENKLAVE_ENKLAVE_HPP

#include <algorithm>
#include <chrono>
#include <fstream>
#include <numeric>
#include <optional>
#include <regex>

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
    enum class EnklaveEventType {
        /** Should be used for default initialization if declaration is required before the type of the enklave event
         * is known at runtime.
         */
        UNDEFINED,
        CHECK_IN,
        CHECK_OUT
    };

    /** Concrete class holding values parsed from files used for computations. See \ref EnklaveEventType for possible
     * "enklave event types".
     */
    struct EnklaveEvent {
        EnklaveEventType type = EnklaveEventType::UNDEFINED;
        date::sys_seconds when; // Default initializes to 0 that corresponds to 1970-01-01 00:00:00.
        fs::path file; // Default initializes to empty path.

        bool operator<(const EnklaveEvent &rhs) {
            return this->when < rhs.when;
        }
    };

    /// Pretty-print EnklaveEvent to terminal.
    std::ostream &operator<<(std::ostream &out, const EnklaveEvent &event) {
        switch (event.type) {
            case EnklaveEventType::UNDEFINED:
                out << "Undefined EnklaveEventType at: " << date::format("%F %T", event.when) << std::endl;
                break;
            case EnklaveEventType::CHECK_IN :
                out << "Check-in  at: " << date::format("%F %T", event.when) << std::endl;
                break;
            case EnklaveEventType::CHECK_OUT :
                out << "Check-out  at: " << date::format("%F %T", event.when) << std::endl;
                break;
            default:
                std::cerr << "Output stream not implemented for this EnklaveEventType." << std::endl;
                break;
        }

        event.file.empty() ? std::cerr << "Path to file is empty." : out << "File: " << event.file << std::endl;
        return out;
    }

    /// Timeslots consist in a pair of a check-in and a check-out contained in EnklaveEvent.
    using timeslot = std::pair<EnklaveEvent &, EnklaveEvent &>;

    /** Convert a string of a very specific form containing a datetime to date::sys_seconds.
     *
     * The first 16 characters of the string are ignored and the remaining rest parsed by the used date-library.
     * Valid input strings have, e.g. this form: "X-Pm-Date: Fri, 13 Sep 2019 13:44:02 +0200".
     *
     * If the string can't be converted to date::sys_seconds or is shorter then 16 character, an empty optional is
     * returned.
     *
     * Note: timezones will be stripped away.
     *
     * Very unlikely, but possible, this function can throw a bad_alloc exception.
     *
     * @param input String containing a datetime.
     * @return std::optional<date::sys_seconds>.
    */
    std::optional<date::sys_seconds> parse_datetime(const std::string &line) noexcept(false) {
        date::sys_seconds parsed_sys_seconds;

        // Extract substring containing the datetime from line.
        std::string datetime;
        try {
            datetime = line.substr(16, line.size());
        } catch (std::out_of_range &e) { // e.g. if line.size() < 16.
            std::cerr << e.what() << std::endl;
            return std::nullopt;
        } // Let caller catch all other exceptions.

        std::stringstream extracted_date{datetime};
        extracted_date >> date::parse("%d %b %Y %T", parsed_sys_seconds);

        if (extracted_date.fail()) {
            return std::nullopt;
        } else {
            return parsed_sys_seconds;
        }
    }


    /** Parse a file from top to bottom line-by-line.
     *
     * The returned object contains the information if it was a check-in or a check-out and when it happened.
     * This function can throw runtime_errors for various reasons and thus will either throw or return a value.
     *
     * @param f Path to a file
     * @return EnklaveEvent.
     */
    EnklaveEvent parse_file(const fs::path &f) noexcept(false) {
        // Regex expressions used to extract required values from file.

        // File is from enklave (and thus relevant) if it contains a line with "header.from=enklave.de".
        const std::regex is_from_enklave_regex{"header.from=enklave.de"};

        // File is a check-in if it contains a line that starts with "Subject:" and contains "Check_in".
        const std::regex check_in_regex("(^Subject).*Check_in");

        // File is a check-out if it contains a line that starts with "Subject:" and contains "Check out".
        const std::regex check_out_regex("(^Subject).*Check out");

        // The datetime that should be used is contained in a line that starts with "X-Pm-Date:".
        const std::regex date_regex("^X-Pm-Date:");

        EnklaveEvent result;
        std::ifstream ifs{f};
        std::string line;
        bool isCheckIn = false;
        bool isCheckOut = false;

        if (!getline(ifs, line)) {
            throw std::runtime_error{"Could not open file or get the first line: " + f.string()};
        }

        // First line in file must contain is_from_enklave_regex.
        if (!regex_search(line, is_from_enklave_regex)) {
            throw std::runtime_error{"Parsed file is not an email from enklave: " + f.string()};
        }

        /* After the first line was parsed, read the rest of the file from top to bottom and assume:
         * - First check_in_regex OR check_out_regex appears in file determining which event it was.
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
                    throw std::runtime_error{"Parsed file is neither a check-in nor a check-out: " + f.string()};
                } // Assume no file that is a check-in AND a check-out exists.

                // Parse datetime.
                auto datetime = parse_datetime(line);
                if (!datetime) {
                    throw std::runtime_error{"Datetime could not be parsed: " + f.string()};
                }

                if (isCheckIn)
                    result = EnklaveEvent{EnklaveEventType::CHECK_IN, datetime.value(), f};
                if (isCheckOut)
                    result = EnklaveEvent{EnklaveEventType::CHECK_OUT, datetime.value(), f};

                // Above runtime_erros cover parsing errors such that no sanity check on result is implemented here.
            }
        }
        return result;
    }

    /** Read all files in a directory and return a vector with parsed data.
     *
     * Calls \ref parse_file for each file, collects the results and catches exceptions that should not stop the
     * program.
     *
     * Only files with with extension ".eml" are considered. Subdirectories are not included.
     *
     * @param p Path do a directory.
     * @return Vector of EnklaveEvent.
     */
    std::vector<EnklaveEvent> parse_directory(const fs::path &p) {
        std::cout << "Scanning for relevant files in: " << p << ":\n";
        std::vector<EnklaveEvent> enklave_events;
        for (const fs::directory_entry &x: fs::directory_iterator(p)) {
            const fs::path &f = x;
            if (f.extension() == ".eml") {
                try {
                    enklave_events.push_back(parse_file(f));
                } catch (std::runtime_error &e) {
                    std::cerr << e.what() << std::endl; // e.g. file could be opened, but parsing did not meet criteria.
                } // Let the caller catch all other exceptions, e.g. from filesystem.
            }
        }
        return enklave_events;
    }

    /** Match check-ins to corresponding check-outs in pairs.
     *
     * First, the input vector is sorted according to the timestamp (datetime) of the events.
     *
     * As result, forgotten check-in's or check-out's appear as adjacent events of the same \ref EnklaveEventType
     * time-sorted vector.
     * Only the last (oldest) event is kept and younger adjacent events are deleted.
     *
     * @param Vector with EnklaveEvents.
     * @return Vector with /ref timeslot.
     */
    std::vector<timeslot> compute_timeslots(std::vector<EnklaveEvent> &events) noexcept(false) {
        std::vector<timeslot> result;

        if (events.size() < 2) {
            throw std::logic_error("At least 2 events must be provided.");
        }

        // Sort by time.
        sort(events.begin(), events.end());

        auto impossible_event_predicate = [](const EnklaveEvent &first, const EnklaveEvent &second) {
            // If both events are of same type.
            return (first.type == EnklaveEventType::CHECK_IN && second.type == EnklaveEventType::CHECK_IN) ||
                   (first.type == EnklaveEventType::CHECK_OUT && second.type == EnklaveEventType::CHECK_OUT);
        };

        // Forgotten events require filtering; see documentation of this function.
        auto it = events.begin();
        do {
            it = adjacent_find(it, events.end(), impossible_event_predicate);
            if (it != events.end()) {
                std::cerr << "The following event is impossible and thus removed from computation:" << std::endl;
                std::cerr << *it;
                it = events.erase(it); // TODO Moving elements out of the vector is most probably more efficient.
            }

        } while (it != events.end());

        if (events.size() % 2 != 0) {
            std::cerr << "The last event in the list is removed, it misses its check-out." << std::endl;
            std::cerr << events.back();
            events.pop_back();
        }

        /* Above check ensures the manual loop will always terminate.
         *
         * Note: this loop iterates on container type EnklaveEvent.
         */
        for (auto check_in = events.begin(); check_in != events.end(); ++check_in) {
            auto check_out = next(check_in);

            if (check_out == events.end())
                throw std::logic_error("Unexpected logic error when computing timeslots.");

            result.emplace_back(timeslot{*check_in, *check_out});
            ++check_in;
        }

        // Do some sanity checks of result.

        if (events.size() / 2 != result.size()) {
            throw std::logic_error("The computed timeslots must be half of the size of all check-ins and check-outs.");
        }

        for (auto &e: result) {
            if (e.first.type != EnklaveEventType::CHECK_IN || e.second.type != EnklaveEventType::CHECK_OUT) {
                throw std::logic_error(
                        "An unexpected logic error occurred: one or more check-ins and/or check-outs are "
                        "interchanged.");
            }
        }
        return result;
    }

    /** Compute the time spend at Enklave.
     *
     * @param Vector with \ref timeslot
     * @return std::chrono::seconds
     */
    std::chrono::seconds compute_duration(const std::vector<timeslot> &slots) {
        using namespace std::literals;
        return std::accumulate(slots.begin(), slots.end(), 0s, [](std::chrono::seconds accumulator, timeslot slot) {
            return accumulator + (slot.second.when - slot.first.when);
        });
    }
}
#endif //TIME_AT_ENKLAVE_ENKLAVE_HPP
