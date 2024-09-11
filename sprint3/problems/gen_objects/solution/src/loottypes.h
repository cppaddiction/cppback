#pragma once
#include <unordered_map>
#include <boost/json.hpp>

namespace loot_types {
	inline std::unordered_map<std::string, boost::json::array>  to_frontend_loot_type_data;
}