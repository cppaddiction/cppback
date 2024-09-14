#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/matchers/catch_matchers.hpp>

// Напишите здесь тесты для функции collision_detector::FindGatherEvents

const double item_width = 0.2;
const double gatherer_width = 0.3;

enum class mode {
	BL,
	BR,
	UL,
	UR,
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
			gatherers_.emplace_back(geom::Point2D{ 0.0, 0.6 }, geom::Point2D{ 5.0, 0.6 }, gatherer_width);
			gatherers_.emplace_back(geom::Point2D{ 1.0, -1.0 }, geom::Point2D{ 1.0, 5.0 }, gatherer_width);
		}
		else if (md == mode::BR)
		{
			gatherers_.emplace_back(geom::Point2D{ 5.0, 0.6 }, geom::Point2D{ 0.0, 0.6 }, gatherer_width);
			gatherers_.emplace_back(geom::Point2D{ 4.0, -1.0 }, geom::Point2D{ 4.0, 5.0 }, gatherer_width);
		}
		else if (md == mode::UL)
		{
			gatherers_.emplace_back(geom::Point2D{ 0.0, 4.4 }, geom::Point2D{ 5.0, 4.4 }, gatherer_width);
			gatherers_.emplace_back(geom::Point2D{ 1.0, 6.0 }, geom::Point2D{ 1.0, 0.0 }, gatherer_width);
		}
		else
		{
			gatherers_.emplace_back(geom::Point2D{ 5.0, 4.4 }, geom::Point2D{ 0.0, 4.4 }, gatherer_width);
			gatherers_.emplace_back(geom::Point2D{ 4.0, 6.0 }, geom::Point2D{ 4.0, 0.0 }, gatherer_width);
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

struct SpecialMatcherOne : Catch::Matchers::MatcherGenericBase {
	SpecialMatcherOne() = default;
	SpecialMatcherOne(SpecialMatcherOne&&) = default;

	template <typename OtherRange>
	bool match(OtherRange other) const {
		using std::begin;
		using std::end;
		auto res1 = collision_detector::TryCollectPoint(geom::Point2D{ 0.0, 0.6 }, geom::Point2D{ 5.0, 0.6 }, geom::Point2D{ 1.0, 1.0 });
		auto res2 = collision_detector::TryCollectPoint(geom::Point2D{ 1.0, -1.0 }, geom::Point2D{ 1.0, 5.0 }, geom::Point2D{ 1.0, 1.0 });
		auto res3 = collision_detector::TryCollectPoint(geom::Point2D{ 0.0, 0.6 }, geom::Point2D{ 5.0, 0.6 }, geom::Point2D{ 4.0, 1.0 });
		auto res4 = collision_detector::TryCollectPoint(geom::Point2D{ 1.0, -1.0 }, geom::Point2D{ 1.0, 5.0 }, geom::Point2D{ 1.0, 4.0 });
		bool res1_collected = res1.IsCollected(item_width + gatherer_width);
		bool res2_collected = res2.IsCollected(item_width + gatherer_width);
		bool res3_collected = res3.IsCollected(item_width + gatherer_width);
		bool res4_collected = res4.IsCollected(item_width + gatherer_width);
		if (!(res1_collected && res2_collected && res3_collected && res4_collected))
		{
			return false;
		}
		else
		{
			auto res1_by_yandex = begin(other);
			auto res2_by_yandex = std::next(res1_by_yandex, 1);
			auto res3_by_yandex = std::next(res2_by_yandex, 1);
			auto res4_by_yandex = std::next(res3_by_yandex, 1);
			if (!(res1_by_yandex->sq_distance == res1.sq_distance && res1_by_yandex->time == res1.proj_ratio && res1_by_yandex->item_id == 0 && res1_by_yandex->gatherer_id == 0))
			{
				return false;
			}
			if (!(res2_by_yandex->sq_distance == res2.sq_distance && res2_by_yandex->time == res2.proj_ratio && res2_by_yandex->item_id == 0 && res2_by_yandex->gatherer_id == 1))
			{
				return false;
			}
			if (!(res3_by_yandex->sq_distance == res3.sq_distance && res3_by_yandex->time == res3.proj_ratio && res3_by_yandex->item_id == 1 && res3_by_yandex->gatherer_id == 0))
			{
				return false;
			}
			if (!(res4_by_yandex->sq_distance == res4.sq_distance && res4_by_yandex->time == res4.proj_ratio && res4_by_yandex->item_id == 2 && res4_by_yandex->gatherer_id == 1))
			{
				return false;
			}
		}
		return true;
	}

	std::string describe() const override {
		// Описание свойства, проверяемого матчером:
		using namespace std::literals;
		return "SpecialMatcherOne"s;
	}
};

UniversalMatcher GetUniversalMatcher() {
    return UniversalMatcher{};
}

SpecialMatcherOne GetSpecialMatcherOne() {
	return SpecialMatcherOne{};
}

struct DefaultEvents1 {
	TestItemGathererProvider prov{};
};

struct DefaultEvents2 {
	TestItemGathererProvider prov{};
};

struct DefaultEvents3 {
	TestItemGathererProvider prov{};
};

struct DefaultEvents4 {
	TestItemGathererProvider prov{};
};

namespace {
	const std::string DefaultEventsTag = "[DET]";
}

TEST_CASE_METHOD(DefaultEvents1, "Everything is ok", DefaultEventsTag) {
	prov.PlaceGatherers(mode::BL);
	std::vector<collision_detector::GatheringEvent> result = collision_detector::FindGatherEvents(prov);
	CHECK_THAT(result, GetUniversalMatcher());
	CHECK_THAT(result, GetSpecialMatcherOne());
}

TEST_CASE_METHOD(DefaultEvents2, "Everything is ok", DefaultEventsTag) {
	prov.PlaceGatherers(mode::BR);
	std::vector<collision_detector::GatheringEvent> result = collision_detector::FindGatherEvents(prov);
	CHECK_THAT(result, GetUniversalMatcher());
}

TEST_CASE_METHOD(DefaultEvents3, "Everything is ok", DefaultEventsTag) {
	prov.PlaceGatherers(mode::UL);
	std::vector<collision_detector::GatheringEvent> result = collision_detector::FindGatherEvents(prov);
	CHECK_THAT(result, GetUniversalMatcher());
}

TEST_CASE_METHOD(DefaultEvents4, "Everything is ok", DefaultEventsTag) {
	prov.PlaceGatherers(mode::UR);
	std::vector<collision_detector::GatheringEvent> result = collision_detector::FindGatherEvents(prov);
	CHECK_THAT(result, GetUniversalMatcher());
}