#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include <fstream>
#include <sstream>

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);
constexpr float MAX_DODGE_TIME = 1.2f;
constexpr int32_t FILE_VERSION = 2; // update this when changing file variables (beacuse a difference in the order/quantity of data being written/read will break the whole process)

#include "TrainingPack.h"

// Bullshit for Saving and Loading packs
template<typename T>
void writePOD(std::ostream& out, const T& t) {
	out.write(reinterpret_cast<const char*>(&t), sizeof(T));
}

template<typename T>
void readPOD(std::istream& in, T& t) {
	in.read(reinterpret_cast<char*>(&t), sizeof(T));
}


class JSutsTeamTraining: public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow, public BakkesMod::Plugin::PluginWindow
{

	//std::shared_ptr<bool> enabled;

	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	void createDrillFromCurrentReplayPosition();	
	void loadPack(std::vector<std::string> args); //--> TrainingPack instantiation
	void savePack();
	
	void loadDrill();



private:
	/* Data currently loaded in memory */
	TrainingPack currentPack;
	DrillData currentDrill;
	float lastCaptureTime = 0;
	float lastApplyTime = 0;
	bool training = false;
	bool record = false;
	bool mirror = false;
	int position = 0;
	int rep = 1;
	
	void cyclePlayers();
	void cycleDrills();
	void setupDrillHook(std::string eventname);
	void createDrillHook(std::string eventname);

	// Inherited via PluginSettingsWindow
	void RenderSettings() override;
	std::string GetPluginName() override;
	// void SetImGuiContext(uintptr_t ctx) override;
	
	// Inherited via PluginWindow

	bool isWindowOpen_ = false;
	bool isMinimized_ = false;
	std::string menuTitle_ = "JSutsTeamTraining";

	virtual void Render() override;
	virtual std::string GetMenuName() override;
	virtual std::string GetMenuTitle() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
};

