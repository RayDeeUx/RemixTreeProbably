#pragma once

using namespace geode::prelude;

using GDHistoryRemixTreeTask = geode::Task<geode::Result<std::string, std::string>>;

namespace Utils {
	GDHistoryRemixTreeTask fetchRemixesForLevelID();
}