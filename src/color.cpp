#include "color.hpp"
#include "json.hpp"

//--------------------------------------------------------------------------------------------------
void bkrl::load_definitions(
    color_dictionary& dic
  , bklib::utf8_string_view const data
  , detail::load_from_string_t
) {
    load_definitions<color_def>(data, [&](color_def const& def) {
        dic.insert_or_replace(def); // TODO duplicates
        return true;
    });
}
