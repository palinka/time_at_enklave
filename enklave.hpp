#ifndef TIME_AT_ENKLAVE_ENKLAVE_HPP
#define TIME_AT_ENKLAVE_ENKLAVE_HPP

#include <fstream>
#include <istream>
#include <regex>
#include <chrono>
#include <set>
// TODO Switch to a newer compiler.
#include <experimental/filesystem>
#include <optional>
#include <algorithm>
#include <numeric>
// TODO Document usage of date-lib.
#include "include/date.h"
#include "config.hpp"

namespace enklave {
    using namespace std;

    // TODO: document types!
    using check_in_datetime = date::sys_seconds;
    using check_out_datetime = date::sys_seconds;
    using enklave_slot = pair<check_in_datetime, check_out_datetime>;
    using duration = std::chrono::duration<int>; // Let's use int for durations.

    namespace fs = experimental::filesystem;

/**
 * @param folder Folder containing downloaded emails (*.eml).
 * @param relevant_files_regex Regex determining which files are considered.
 * @return Vector containing filesystem paths of relevant mails. Empty vector if none is found in folder.
 */
    vector<fs::path> get_relevant_files(const fs::path folder, std::regex relevant_files_regex) noexcept(false) {
        // Throw, e.g. if the directory is not found et.all.
        vector<fs::path> relevant_files;
        cout << "Looking for relevant files in: " << folder << ":\n";
        for (const fs::directory_entry &f : fs::directory_iterator{folder}) {
            // Search for pattern in filename and add to vector if matched.
            if (regex_search(f.path().filename().string(), relevant_files_regex))
                relevant_files.push_back(f);
        }
        return relevant_files;
    }

/**
 * Converts a string containing a ISO 8601-like date to the date::sys_seconds.
 *
 * ISO 8601-like strings that use "_" instead of ":" are valid input, e.g. "Confirmation 2019-08-22T15_31_05+02_00.eml".
 *
 * Returns an optional to avoid any confusions with '0' which is a valid value for date::sys_seconds.
 *
 * @param str String in ISO 8601-like format.
 * @return std::optional<date::sys_seconds> in UTC.
 */
    std::optional<date::sys_seconds> parse_datetime(const string &input) {
        // First, reduce input string to datetime digits only.
        smatch matches_date_time; // Contains result.
        const regex date_time_pattern{R"(\d{4}-\d{2}-\d{2}T\d{2}_\d{2}_\d{2}\+\d{2}_\d{2})"};
        regex_search(input, matches_date_time, date_time_pattern);

        // Convert string to ISO format by replacing "_" with ":"
        istringstream iso_string(regex_replace(string{matches_date_time[0]}, regex("_"), ":"));

        // Convert to sys_seconds now that we have an ISO string.
        date::sys_seconds point_in_time;
        // Note: %z includes the timezone and thus has the effect that UTC will be returned.
        iso_string >> date::parse("%FT%T%z", point_in_time);
        if (bool(iso_string)) {
            return point_in_time;
        } else {
            return nullopt; // empty optional.
        }
    }

    /**
     *
     * @param p Vector containing filesystem paths for parsing.
     * @return Vector with enklave_slots. TODO make link to type.
     */
    vector<enklave_slot> parse(const vector<fs::path> &p) noexcept(false) {
        using namespace date; // use also overload for <<
        set<check_in_datetime> check_ins;
        set<check_out_datetime> check_outs;

        // Parse datetime and add results to two sets containing all check-ins and check-outs.
        // Using two sets makes code very readable (to me); paying the price of extra space for the result vector.
        // TODO Maybe std::for_each + a lambda is more elegant.
        for (auto &x : p) {
            string filename = x.filename().string();
            // If datetime was parsed successfully.
            if (auto datetime = parse_datetime(filename)) {
                // If its was a checkin.
                if (regex_search(filename, enklave::config::check_in_pattern))
                    check_ins.insert({datetime.value()});
                // If it was a checkout.
                if (regex_search(filename, enklave::config::check_out_pattern)) {
                    check_outs.insert({datetime.value()});
                }
            }
        }

        // TODO if a one of the two was forgotten, at least show user when!
        if (check_ins.size() != check_outs.size())
            throw "Number of check-ins and check-outs do not match!";


        // A pair of iterators to the sets could also be returned; for now I prefer to zip them in a vector.
        vector<enklave_slot> slots(check_ins.size());
        std::transform(check_ins.begin(), check_ins.end(), check_outs.begin(), std::back_inserter(slots),
                       [](auto check_in, auto check_out) {
                           return make_pair(check_in, check_out);
                       });

        return slots;
    }

    duration compute_duration(const vector<enklave_slot> &slots) {
        return std::accumulate(slots.begin(), slots.end(), 0s, [](duration accumulator, auto slot) {
            return accumulator + (slot.second - slot.first);
        });
    }
}
#endif //TIME_AT_ENKLAVE_ENKLAVE_HPP
