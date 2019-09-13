#include "gtest/gtest.h"
#include "../enklave.hpp"
#include "../config.hpp"

using namespace enklave;

// TODO: pass timezone offset to UTC via flags.
TEST(parseDatetime, WithSuccess) {
    using namespace date;

    // Convert a point in time from string to sys_seconds (alias for std::chrono::point_in_time_string).
    auto point_in_time = parse_datetime("Confirmation_  Check in 2019-08-22T15_31_05+02_00.eml").value();

    // Reconvert to a string.
    // Add manually the timezone difference; automation not offered by used lib.
    auto point_in_time_string = format("%F %T", point_in_time + 2h);
    EXPECT_EQ("2019-08-22 15:31:05", point_in_time_string);
}

TEST(parseDatetime, WithFailure) {
    EXPECT_EQ(parse_datetime("somestring"), std::nullopt);
    // Test an impossible date.
    EXPECT_EQ(parse_datetime("2019-99-22T15_31_05+02_00"), std::nullopt);
}

// This test is rather poor. However, it assumes 5 valid files in configured folder.
TEST(getRelevantFilePathsFromFolder, WithSuccess) {
    auto files = get_relevant_files(enklave::config::path_with_mails, enklave::config::relevant_files_regex);
    EXPECT_EQ(files.size(), 4);
}

TEST(getRelevantFilePathsFromFolder, FolderNotFound) {
    EXPECT_THROW(
            get_relevant_files("someFolderThatSHOULDnotExist/never/ever", enklave::config::relevant_files_regex),
            std::experimental::filesystem::filesystem_error);
}
