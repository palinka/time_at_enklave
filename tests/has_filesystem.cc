// Test used by cmake at configuration time to check if filesystem header is available.
// https://stackoverflow.com/questions/54290254/problem-adding-stdfilesystem-to-cmake-project

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

int main()
{
    fs::path aPath {"../"};

    return 0;
}