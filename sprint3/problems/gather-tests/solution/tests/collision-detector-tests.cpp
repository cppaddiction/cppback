#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/matchers/catch_matchers.hpp>
// Напишите здесь тесты для функции collision_detector::FindGatherEvents

const double item_width = 0.3;
const double gatherer_width = 0.6;

enum class mode {
	BL,
	BR,
	UL,
	UR
};

class TestItemGathererProvider: public collision_detector::ItemGathererProvider {
public:
	TestItemGathererProvider() {
		items_.emplace_back(geom::Point2D{1.0, 1.0}, item_width);
		items_.emplace_back(geom::Point2D{4.0, 1.0}, item_width);
		items_.emplace_back(geom::Point2D{1.0, 4.0}, item_width);	
		items_.emplace_back(geom::Point2D{4.0, 4.0}, item_width);
		//objects (items) to gather | square configuration
	}
	void PlaceGatherers(mode md) {
		if (md == mode::BL)
		{
			gatherers_.emplace_back(geom::Point2D{ 0.0, 1.0 }, geom::Point2D{ 5.0, 1.0 }, gatherer_width);
			gatherers_.emplace_back(geom::Point2D{ 1.0, 0.0 }, geom::Point2D{ 1.0, 5.0 }, gatherer_width);
		}
		else if (md == mode::BR)
		{
			gatherers_.emplace_back(geom::Point2D{ 5.0, 1.0 }, geom::Point2D{ 0.0, 1.0 }, gatherer_width);
			gatherers_.emplace_back(geom::Point2D{ 4.0, 0.0 }, geom::Point2D{ 4.0, 5.0 }, gatherer_width);
		}
		else if (md == mode::UL)
		{
			gatherers_.emplace_back(geom::Point2D{ 0.0, 4.0 }, geom::Point2D{ 5.0, 4.0 }, gatherer_width);
			gatherers_.emplace_back(geom::Point2D{ 1.0, 5.0 }, geom::Point2D{ 1.0, 0.0 }, gatherer_width);
		}
		else
		{
			gatherers_.emplace_back(geom::Point2D{ 5.0, 4.0 }, geom::Point2D{ 0.0, 4.0 }, gatherer_width);
			gatherers_.emplace_back(geom::Point2D{ 4.0, 5.0 }, geom::Point2D{ 4.0, 0.0 }, gatherer_width);
		}
		//place gatherers in a special way | each gatherer collects 2 objects (items)
	}
	size_t ItemsCount() const override {
		return items_.size();
	}
	collision_detector::Item GetItem(size_t idx) const override {
		return items_[idx];
	}
	size_t GatherersCount() const override {
		return gatherers_.size();
	}
	collision_detector::Gatherer GetGatherer(size_t idx) const override {
		return gatherers_[idx];
	}
private:
	std::vector< collision_detector::Item> items_;
	std::vector< collision_detector::Gatherer> gatherers_;
};

struct UniversalMatcher : Catch::Matchers::MatcherGenericBase {
	UniversalMatcher() = default;
	UniversalMatcher(UniversalMatcher&&) = default;

    template <typename OtherRange>
    bool match(OtherRange other) const {
		using std::begin;
		using std::end;
		if (std::distance(begin(other), end(other)) != 4)
		{
			return false;
		}
		for (auto it = begin(other); it != end(other); it++)
		{
			if (it->item_id < 0 || it->gatherer_id < 0 || it->sq_distance < 0 || it->time < 0)
			{
				return false;
			}
		}
		for (auto it = std::next(begin(other), 1); it != end(other); it++)
		{
			auto prev = std::next(it, -1);
			if (it->time < prev->time)
			{
				return false;
			}
		}
		return true;
    }

    std::string describe() const override {
        // Описание свойства, проверяемого матчером:
		using namespace std::literals;
		return "UniversalMatcher"s;
    }
};

UniversalMatcher GetUniversalMatcher() {
    return UniversalMatcher{};
}

struct DefaultEvents1 {
	TestItemGathererProvider prov{};
	prov.PlaceGatherers(mode::BL);
	std::vector<collision_detector::GatheringEvent> result = collision_detector::FindGatherEvents(prov);
};

struct DefaultEvents2 {
	TestItemGathererProvider prov{};
	prov.PlaceGatherers(mode::BR);
	std::vector<collision_detector::GatheringEvent> result = collision_detector::FindGatherEvents(prov);
};

struct DefaultEvents3 {
	TestItemGathererProvider prov{};
	prov.PlaceGatherers(mode::UL);
	std::vector<collision_detector::GatheringEvent> result = collision_detector::FindGatherEvents(prov);
};

struct DefaultEvents4 {
	TestItemGathererProvider prov{};
	prov.PlaceGatherers(mode::UR);
	std::vector<collision_detector::GatheringEvent> result = collision_detector::FindGatherEvents(prov);
};

namespace {
	const std::string DefaultEventsTag = "[DET]";
}

TEST_CASE_METHOD(DefaultEvents1, "Everything is ok", DefaultEventsTag) {
	CHECK_THAT(result, GetUniversalMatcher());
}

TEST_CASE_METHOD(DefaultEvents2, "Everything is ok", DefaultEventsTag) {
	CHECK_THAT(result, GetUniversalMatcher());
}

TEST_CASE_METHOD(DefaultEvents3, "Everything is ok", DefaultEventsTag) {
	CHECK_THAT(result, GetUniversalMatcher());
}

TEST_CASE_METHOD(DefaultEvents4, "Everything is ok", DefaultEventsTag) {
	CHECK_THAT(result, GetUniversalMatcher());
}