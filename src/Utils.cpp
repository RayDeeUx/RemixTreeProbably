#include "Manager.hpp"
#include "Utils.hpp"

using namespace geode::cocos;

namespace Utils {
	GDHistoryRemixTreeTask fetchRemixesForLevelID() {
		geode::utils::web::WebRequest gdHistoryRequest = geode::utils::web::WebRequest();
		return gdHistoryRequest.get(fmt::format("https://history.geometrydash.eu/api/v1/search/level/advanced/?filter=cache_original%3D{}&limit=100", Manager::get()->levelID)).map([] (geode::utils::web::WebResponse* response) -> Result<std::string> {
			Manager::get()->actualHits = 0;

			if (!response) {
				log::info("web response is nullptr");
				return Err("Something went wrong... (Nullptr.)");
			}

			if (!response->ok() || response->code() != 200) {
				log::info("GDHistory request failed: {}", response->code());
				return Err("Something went wrong... (Not HTTP code 200.)");
			}

			auto responseString = response->string();
			if (responseString.isErr()) {
				log::info("GDHistory response was not a string");
				return Err("Something went wrong... (Not a string!)");
			}

			auto responseUnwrapped = responseString.unwrap();
			if (responseUnwrapped.empty()) {
				log::info("GDHistory response was a string, but unwrapping it failed.");
				return Err("Something went wrong... (Couldn't unwrap string!)");
			}

			auto responseAsJSON = matjson::parse(responseString.unwrap());
			if (responseAsJSON.isErr()) {
				log::info("GDHistory response was a JSON string, but parsing it failed.");
				return Err("Something went wrong... (Couldn't JSON-ify string!)");
			}

			auto responseAsJSONUnwrapped = responseAsJSON.unwrap();

			if ((responseAsJSONUnwrapped.contains("success") && !responseAsJSONUnwrapped["success"].asBool().unwrapOr(false)) || responseAsJSONUnwrapped.contains("error")) {
				log::info("GDHistory response was a JSON string, but GDHistory returned an error.");
				return Err("Something went wrong... (Unwrapped JSON-ified string, but GDHistory returned an error!)");
			}

			if (!responseAsJSONUnwrapped.contains("hits") || !responseAsJSONUnwrapped.contains("estimatedTotalHits")) {
				log::info("GDHistory response was a JSON string, but no relevant data found.");
				return Err("Something went wrong... (Unwrapped JSON-ified string, but no relevant data found!)");
			}

			auto estTotal = responseAsJSONUnwrapped["estimatedTotalHits"].asInt();
			if (estTotal.isErr()) {
				log::info("GDHistory response has estimatedTotalHits, but parsing it failed.");
				return Err("Something went wrong... (Found results, but couldn't parse it!)");
			}

			auto hitsArray = responseAsJSONUnwrapped["hits"].asArray();

			if (hitsArray.isErr()) {
				log::info("GDHistory response has relevant data, but parsing it failed.");
				return Err("Something went wrong... (Found results, but couldn't parse it!)");
			}

			auto unwrappedHits = hitsArray.unwrap();
			if (unwrappedHits.empty()) {
				log::info("GDHistory response has relevant data, but there was nothing.");
				return Err("No one has remixed or cloned this level.\n\n<cy>That's either good news or bad news... Who knows?</c>");
			}

			std::vector<int> levelIDRemixes = {};
			for (matjson::Value entry : unwrappedHits) {
				if (!entry.contains("online_id") || entry["online_id"].asInt().isErr() || entry["online_id"].asInt().unwrap() == Manager::get()->levelID) continue;

				if (entry.contains("is_public") && entry["is_public"].asBool().isOk() && !entry["is_public"].asBool().unwrap()) continue;
				if (entry.contains("is_deleted") && entry["is_deleted"].asBool().isOk() && entry["is_deleted"].asBool().unwrap()) continue;

				levelIDRemixes.push_back(static_cast<int>(entry["online_id"].asInt().unwrap()));
			}

			if (levelIDRemixes.empty()) {
				log::info("GDHistory response has relevant data, but all the levels have been deleted or are private.");
				return Err("No one has made a <cj>public</c> remix of this level.\n\n<cy>That's either good news or bad news... Who knows?</c>");
			}

			Manager::get()->actualHits = estTotal.unwrap();
			return Ok(fmt::format("{}", fmt::join(levelIDRemixes.begin(), levelIDRemixes.end(), ",")));
		},
		[](auto) -> std::monostate {
			return {};
		});
	}
}