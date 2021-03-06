#include "gtest/gtest.h"
#include "../enklave.hpp"
#include "../config.hpp"

using namespace enklave;
// Filesystem needs some care on different compilers.
#include <filesystem>

#ifdef _WIN32
namespace fs = std::experimental::filesystem::v1;
#elif __linux__
namespace fs = std::filesystem;
#endif

TEST(parseDatetime, WithSuccess) {
    // Convert a string containing a point in time to date::sys_seconds.
    auto point_in_time = parse_datetime("X-Pm-Date: Fri, 13 Sep 2019 13:44:02 +0200").value();

    // Reconvert result to a string.
    auto point_in_time_string = date::format("%F %T", point_in_time);
    EXPECT_EQ("2019-09-13 13:44:02", point_in_time_string);
}

TEST(parseDatetime, WithFailure) {
    // Test some "random" input.
    EXPECT_EQ(parse_datetime("somestring"), std::nullopt);
    // Test an impossible date.
    EXPECT_EQ(parse_datetime("X-Pm-Date: Fri, 99 Sep 2019 13:44:02 +0200"), std::nullopt);
    // Test an almost valid input.
    EXPECT_EQ(parse_datetime("Pm-Date: Fri, 13 Sep 2019 13:44:02 +0200"), std::nullopt);
}

TEST(parseFile, IsCheckinAndNotCheckout) {
    date::sys_seconds zero; // default init.
    auto result = parse_file(std::string{enklave::config::path_with_mails} + "/testfile_check_in_01.eml");
    EXPECT_NE(result.when, zero);
}


TEST(parseFile, FileNotFound) {
    EXPECT_THROW(parse_file("someFolderThatSHOULDnotExist/never/ever"), std::runtime_error);
}

TEST(parseFile, SomeOtherFileFromEnklave) {
    EXPECT_THROW(parse_file(std::string{enklave::config::path_with_mails} + "/testfile_enklave_other.eml"),
                 std::runtime_error);
}

TEST(parseFile, ManualyComputeResultOfOneCheckinAndCheckout) {
    using namespace date;
    auto in = parse_file(std::string{enklave::config::path_with_mails} + "/testfile_check_in_01.eml").when;
    auto out = parse_file(std::string{enklave::config::path_with_mails} + "/testfile_check_out_01.eml").when;
    auto result = out - in;
    EXPECT_EQ("04:36:24", format("%T", result));
}

TEST(parseDirectory, WithSuccess) {
    std::vector<EnklaveEvent> results;
    EXPECT_NO_THROW(results = parse_directory(enklave::config::path_with_mails));
}

TEST(parseDirectory, FolderNotFound) {
    EXPECT_THROW(parse_directory("someFolderThatSHOULDnotExist/never/ever"), fs::filesystem_error);
}

TEST(computeTimeslots, WithSuccess) {
    auto results = parse_directory(enklave::config::path_with_mails);
    auto slots = compute_timeslots(results);
    auto[in, out] = slots.front();
    auto first_slot_duration = out.when - in.when;
    EXPECT_EQ("03:36:24", date::format("%T", first_slot_duration));
}

TEST(computeDuration, WithSuccess) {
    auto found_events = parse_directory(enklave::config::path_with_mails);
    auto timeslots = compute_timeslots(found_events);
    auto result = compute_duration(timeslots);
    EXPECT_EQ("11:12:48", date::format("%T", result));
}
