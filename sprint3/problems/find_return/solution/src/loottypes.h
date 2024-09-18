#pragma once
#include <unordered_map>
#include <boost/json.hpp>

namespace loot_types {
	inline std::unordered_map<std::string, boost::json::array>  to_frontend_loot_type_data;
	inline std::unordered_map<std::string, double>  to_frontend_dog_speed_data;
	inline std::unordered_map<std::string, std::uint64_t> to_frontend_bag_capacity_data;
}