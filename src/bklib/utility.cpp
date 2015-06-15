#include "utility.hpp"
#include "assert.hpp"
#include <fstream>

std::vector<char> bklib::read_file_to_buffer(utf8_string_view const filename)
{
    std::ifstream file {filename.data(), std::ios::binary};
    if (!file) {
        BK_ASSERT(false);
    }

    file.seekg(0, std::ios::end);
    std::streamsize const size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> result(size);
    if (!file.read(result.data(), size)) {
        BK_ASSERT(false);
    }

    return result;
}
