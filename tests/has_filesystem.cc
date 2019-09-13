// Test used by cmake at configuration time to check if filesystem header is available.
// https://stackoverflow.com/questions/54290254/problem-adding-stdfilesystem-to-cmake-project

#include <filesystem>

#ifdef _WIN32
namespace fs = std::experimental::filesystem::v1;
#elif __linux__
namespace fs = std::filesystem;
#endif

int main() {
    fs::path aPath{"../"};

    return 0;
}