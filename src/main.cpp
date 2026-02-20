#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/InfoLayer.hpp>
#include "Manager.hpp"
#include "Utils.hpp"

using namespace geode::prelude;

class $modify(MyLevelBrowserLayer, LevelBrowserLayer) {
	bool shouldShowThatsAllFolks(const int value) {
		const int actualHits = static_cast<CCInteger*>(this->getUserObject("remix-tree-actual-hits"_spr))->getValue();
		if (this->m_itemCount < actualHits && value > 10) {
			const long difference = actualHits - this->m_itemCount;
			MDPopup* md = MDPopup::create(
				"That's all, folks!",
				fmt::format(
					"You've hit <cp>the 100 level limit</c> the RemixTreeProbably mod is able to <cg>display and fetch</c> from GDHistory.\n\n"
					"<c-ffff00>__This level limit will not change for a ***very*** long time.__</c>\n\n"
					"## <c-ff0000>***__Do NOT ask if this level limit will be raised!__***</c>\n\n"
					"### There are still <cl>{} more level{}</c> that are <co>clones/remixes</c> of <cj>{} by {}</c> <cb>(Level ID {})</c>.\n"
					"### For the full list of these levels on GDHistory in your web browser, click [here](https://history.geometrydash.eu/search/?original={}).",
					difference, difference != 1 ? "s" : "",
					static_cast<CCString*>(this->getUserObject("remix-tree-level-name"_spr))->getCString(),
					static_cast<CCString*>(this->getUserObject("remix-tree-creator-name"_spr))->getCString(),
					Manager::get()->levelID,
					Manager::get()->levelID
				),
				"I Understand"
			);
			md->m_noElasticity = true,
			md->show();
			return true;
		}
		return false;
	}

	virtual void setIDPopupClosed(SetIDPopup* popup, int value) {
		if (!Mod::get()->getSettingValue<bool>("enabled")) return LevelBrowserLayer::setIDPopupClosed(popup, value);
		if (!popup) return LevelBrowserLayer::setIDPopupClosed(popup, value);
		if (!m_searchObject || m_searchObject->m_searchType != SearchType::Type19) return LevelBrowserLayer::setIDPopupClosed(popup, value);
		if (!this->getUserObject("remix-tree-results"_spr) || !this->getUserObject("remix-tree-actual-hits"_spr)) return LevelBrowserLayer::setIDPopupClosed(popup, value);
		if (!this->getUserObject("remix-tree-level-name"_spr) || !this->getUserObject("remix-tree-creator-name"_spr)) return LevelBrowserLayer::setIDPopupClosed(popup, value);
		if (MyLevelBrowserLayer::shouldShowThatsAllFolks(value)) return LevelBrowserLayer::setIDPopupClosed(popup, 10);
		LevelBrowserLayer::setIDPopupClosed(popup, value);
	}

	void onNextPage(CCObject* sender) {
		if (!Mod::get()->getSettingValue<bool>("enabled")) return LevelBrowserLayer::onNextPage(sender);
		if (!m_searchObject || m_searchObject->m_searchType != SearchType::Type19) return LevelBrowserLayer::onNextPage(sender);
		if (!this->getUserObject("remix-tree-results"_spr) || !this->getUserObject("remix-tree-actual-hits"_spr)) return LevelBrowserLayer::onNextPage(sender);
		if (!this->getUserObject("remix-tree-level-name"_spr) || !this->getUserObject("remix-tree-creator-name"_spr)) return LevelBrowserLayer::onNextPage(sender);
		if (MyLevelBrowserLayer::shouldShowThatsAllFolks(m_searchObject->m_page + 2)) return;
		LevelBrowserLayer::onNextPage(sender);
	}
};

class $modify(MyInfoLayer, InfoLayer) {
	struct Fields {
		async::TaskHolder<GDHistoryRemixTreeTask> listener {};
	};
	void onClose(CCObject* sender) {
		Manager::get()->levelID = 0;
		Manager::get()->actualHits = 0;
		InfoLayer::onClose(sender);
	}
	bool init(GJGameLevel* level, GJUserScore* score, GJLevelList* list) {
		Manager::get()->levelID = 0;
		if (!InfoLayer::init(level, score, list)) return false;
		if (!level) return true;
		if (level->m_levelType == GJLevelType::Editor || level->m_levelType == GJLevelType::Main) return true;
		if (!this->m_mainLayer || !this->m_mainLayer->getChildByID("refresh-menu")) return true;
		if (!Mod::get()->getSettingValue<bool>("enabled")) return true;

		Manager::get()->levelID = level->m_levelID.value();

		CCSprite* newFolder = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
		newFolder->setID("fake-folder-sprite-one"_spr);
		CCSprite* newFolderTwo = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
		newFolderTwo->setID("fake-folder-sprite-two"_spr);
		CCSprite* magnify = CCSprite::createWithSpriteFrameName("edit_findBtn_001.png");
		magnify->setID("fake-search-sprite"_spr);
		EditorButtonSprite* ebs = EditorButtonSprite::create(newFolder, EditorBaseColor::Green);
		ebs->setID("button-sprite-for-two-folders"_spr);
		ebs->addChildAtPosition(newFolderTwo, Anchor::Center, {-2.f, -2.f});
		ebs->addChildAtPosition(magnify, Anchor::Center);
		newFolder->setPosition(newFolder->getPosition() - -2.f);
		newFolder->setScale(.5f);
		newFolderTwo->setScale(.5f);
		magnify->setScale(.45);

		CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(ebs, this, menu_selector(MyInfoLayer::onRemixTree));
		button->setID("view-remixes-clones-button"_spr);
		button->setTag(20260111);
		this->m_mainLayer->getChildByID("refresh-menu")->addChild(button);
		this->m_mainLayer->getChildByID("refresh-menu")->updateLayout();

		return true;
	}
	void onRemixTree(CCObject* sender) {
		if (!Mod::get()->getSettingValue<bool>("enabled")) return;
		if (!m_level || !sender || sender->getTag() != 20260111 || static_cast<CCNode*>(sender)->getID().empty() || static_cast<CCNode*>(sender)->getID() != "view-remixes-clones-button"_spr) return;
		static_cast<CCMenuItemSpriteExtra*>(sender)->setEnabled(false);
		sender->setTag(-1);

		const auto fields = m_fields.self();
		if (!fields) return;
		Manager::get()->levelID = this->m_level->m_levelID.value();

		CCLayerColor* fakeLoadingScreen = CCLayerColor::create({0, 0, 0, 0});
		fakeLoadingScreen->setID("fake-loading-screen"_spr);
		fakeLoadingScreen->ignoreAnchorPointForPosition(false);
		fakeLoadingScreen->setPosition({0.f, 0.f});
		fakeLoadingScreen->setContentSize(CCScene::get()->getContentSize());

		geode::LoadingSpinner* spinner = LoadingSpinner::create(60.f);
		spinner->setID("fake-loading-screen-spinner"_spr);
		spinner->ignoreAnchorPointForPosition(false);
		spinner->setVisible(true);
		spinner->setPosition(CCDirector::get()->getWinSize() / 2.f);
		fakeLoadingScreen->addChild(spinner);

		CCLabelBMFont* loadingText = CCLabelBMFont::create("Fetching from GDHistory...", "bigFont.fnt");
		loadingText->ignoreAnchorPointForPosition(false);
		loadingText->limitLabelWidth(spinner->getContentWidth() * 2.5f, 1.f, .0001f);
		loadingText->setPosition(CCDirector::get()->getWinSize() / 2.f);
		loadingText->setPositionY(loadingText->getPositionY() - 55.f);
		loadingText->setID("fake-loading-screen-text"_spr);
		fakeLoadingScreen->addChild(loadingText);

		fakeLoadingScreen->setPosition(CCDirector::get()->getWinSize() / 2.f);
		fakeLoadingScreen->setZOrder(this->getZOrder() + 1);
		this->getParent()->addChild(fakeLoadingScreen);
		fakeLoadingScreen->runAction(CCFadeTo::create(.1, 64));

		// fields->listener.bind(this, &MyInfoLayer::onFetchRemixTreeSuccess);
		fields->listener.spawn(
			geode::utils::web::WebRequest().get(fmt::format("https://history.geometrydash.eu/api/v1/search/level/advanced/?filter=cache_original%3D{}&limit=100", Manager::get()->levelID)),
			[fakeLoadingScreen](geode::utils::web::WebResponse response) {
				fakeLoadingScreen->removeMeAndCleanup();
				Manager::get()->actualHits = 0;

				if (!response.ok() || response.code() != 200) {
					log::info("GDHistory request failed: {}", response.code());
					return FLAlertLayer::create("Oh no!", "Something went wrong... (Response was not HTTP code 200.)", "I Understand")->show();
				}

				auto responseString = response.string();
				if (responseString.isErr()) {
					log::info("GDHistory response was not a string");
					return FLAlertLayer::create("Oh no!", "Something went wrong... (Response was not a string!)", "I Understand")->show();
				}

				auto responseUnwrapped = responseString.unwrap();
				if (responseUnwrapped.empty()) {
					log::info("GDHistory response was a string, but unwrapping it failed.");
					return FLAlertLayer::create("Oh no!", "Something went wrong... (Couldn't unwrap string!)", "I Understand")->show();
				}

				auto responseAsJSON = matjson::parse(responseString.unwrap());
				if (responseAsJSON.isErr()) {
					log::info("GDHistory response was a JSON string, but parsing it failed.");
					return FLAlertLayer::create("Oh no!", "Something went wrong... (Couldn't JSON-ify string!)", "I Understand")->show();
				}

				auto responseAsJSONUnwrapped = responseAsJSON.unwrap();

				if ((responseAsJSONUnwrapped.contains("success") && !responseAsJSONUnwrapped["success"].asBool().unwrapOr(false)) || responseAsJSONUnwrapped.contains("error")) {
					log::info("GDHistory response was a JSON string, but GDHistory returned an error.");
					return FLAlertLayer::create("Oh no!", "Something went wrong... (Unwrapped JSON-ified string, but GDHistory returned an error!)", "I Understand")->show();
				}

				if (!responseAsJSONUnwrapped.contains("hits") || !responseAsJSONUnwrapped.contains("estimatedTotalHits")) {
					log::info("GDHistory response was a JSON string, but no relevant data found.");
					return FLAlertLayer::create("Oh no!", "Something went wrong... (Unwrapped JSON-ified string, but no relevant data found!)", "I Understand")->show();
				}

				auto estTotal = responseAsJSONUnwrapped["estimatedTotalHits"].asInt();
				if (estTotal.isErr()) {
					log::info("GDHistory response has estimatedTotalHits, but parsing it failed.");
					return FLAlertLayer::create("Oh no!", "Something went wrong... (Found results, but couldn't parse it!)", "I Understand")->show();
				}

				auto hitsArray = responseAsJSONUnwrapped["hits"].asArray();

				if (hitsArray.isErr()) {
					log::info("GDHistory response has relevant data, but parsing it failed.");
					return FLAlertLayer::create("Oh no!", "Something went wrong... (Found results, but couldn't parse it!)", "I Understand")->show();
				}

				auto unwrappedHits = hitsArray.unwrap();
				if (unwrappedHits.empty()) {
					log::info("GDHistory response has relevant data, but there was nothing.");
					return FLAlertLayer::create("Oh no!", "No one has remixed or cloned this level.\n\n<cy>That's either good news or bad news... Who knows?</c>", "I Understand")->show();
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
					return FLAlertLayer::create("Oh no!", "No one has made a <cj>public</c> remix of this level.\n\n<cy>That's either good news or bad news... Who knows?</c>", "I Understand")->show();
				}

				Manager::get()->actualHits = estTotal.unwrap();
				const std::string& listOfLevels = fmt::format("{}", fmt::join(levelIDRemixes.begin(), levelIDRemixes.end(), ","));
				
				CCScene* scene = CCScene::create();
				GJSearchObject* gjso = GJSearchObject::create(SearchType::Type19, listOfLevels);
				LevelBrowserLayer* lbl = LevelBrowserLayer::create(gjso);
				scene->addChild(lbl);

				lbl->setUserObject("remix-tree-results"_spr, CCBool::create(true));
				lbl->setUserObject("remix-tree-actual-hits"_spr, CCInteger::create(static_cast<int>(Manager::get()->actualHits)));
				lbl->setUserObject("remix-tree-level-name"_spr, CCString::create(this->m_level->m_levelName));
				lbl->setUserObject("remix-tree-creator-name"_spr, CCString::create(this->m_level->m_creatorName));

				FLAlertLayer* alert = geode::createQuickPopup(
					"A Friendly Reminder",
					"These results are <co>provided by Cvolton's GDHistory API</c> and are <c_>FOR REFERENCE ONLY</c>.\n\n"
					"By pressing \"I Understand\" or closing this popup, <cg>you agree not to hold anyone besides yourself</c> "
					"<cg>responsible</c> for <cl>your reactions to these search results</c>.\n\n"
					"If you <cr>disagree</c> with this, <cr>please press \"Never Mind\" to go back</c>.",
					"Never Mind", "I Understand", [lbl](auto, const bool btnTwo) {
						if (btnTwo) return;
						if (lbl) return lbl->onBack(nullptr);
						CCDirector::get()->popSceneWithTransition(.5f, PopTransition::kPopTransitionFade);
					}, false
				);
				alert->m_noElasticity = true;
				scene->addChild(alert);
				alert->m_scene = scene;

				if (auto rmx = this->m_mainLayer->getChildByID("refresh-menu")->getChildByID("view-remixes-clones-button"_spr)) {
					static_cast<CCMenuItemSpriteExtra*>(rmx)->setEnabled(true);
					rmx->setTag(20260111);
				}

				CCDirector::get()->pushScene(CCTransitionFade::create(.5f, scene));
			}
		);
	}
};