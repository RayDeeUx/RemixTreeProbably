#pragma once

// Manager.hpp structure by acaruso
// reused with explicit permission and strong encouragement

using namespace geode::prelude;

class Manager {

protected:
	static Manager* instance;
public:

	bool calledAlready = false;

	int levelID = 0;
	long actualHits = 0;

	static Manager* get() {
		if (!instance) instance = new Manager();
		return instance;
	}

};