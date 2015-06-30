#include "utility.hpp"
#include "assert.hpp"
#include "exception.hpp"

#include <fstream>

std::vector<char> bklib::read_file_to_buffer(utf8_string_view const filename)
try {
    std::ifstream file {filename.data(), std::ios::binary | std::ios::in};
    file.exceptions(std::ios_base::badbit | std::ios_base::eofbit | std::ios_base::failbit);

    file.seekg(0, std::ios::end);
    std::streamsize const size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> result(static_cast<size_t>(size));
    file.read(result.data(), size);

    return result;
} catch (std::ios_base::failure& e) {
    // TODO
    BOOST_THROW_EXCEPTION(bklib::io_error {}
        << boost::errinfo_file_name {filename.to_string()}
        << boost::errinfo_errno {e.code().value()}
    );
} catch (...) {
    throw;
}
