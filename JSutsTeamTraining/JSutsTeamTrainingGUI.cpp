#include "pch.h"
#include "JSutsTeamTraining.h"

// Plugin Settings Window code here
std::string JSutsTeamTraining::GetPluginName() {
	return "JSutsTeamTraining";
}

void JSutsTeamTraining::SetImGuiContext(uintptr_t ctx) {
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Render the plugin settings here
// This will show up in bakkesmod when the plugin is loaded at
//  f2 -> plugins -> JSutsTeamTraining
void JSutsTeamTraining::RenderSettings() {
	ImGui::TextUnformatted("JSutsTeamTraining plugin settings");
	if (ImGui::Button("Open Menu"))
	{
		gameWrapper->Execute([this](GameWrapper* gw)
			{
				cvarManager->executeCommand("togglemenu JSutsTeamTraining");
			});
	}
	if (ImGui::Button("Make Drill"))
	{
		gameWrapper->Execute([this](GameWrapper* gw)
			{
				cvarManager->executeCommand("js_training_make_drill");
			});
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Create drill from replay moment.");
	}
	if (ImGui::Button("Load Drill"))
	{
		gameWrapper->Execute([this](GameWrapper* gw)
			{
				cvarManager->executeCommand("js_training_load_drill");
			});
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Run last drill saved to memory");
	}
}


// Do ImGui rendering here
void JSutsTeamTraining::Render()
{
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::End();

	if (!isWindowOpen_)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

// Name of the menu that is used to toggle the window.
std::string JSutsTeamTraining::GetMenuName()
{
	return "JSutsTeamTraining";
}

// Title to give the menu
std::string JSutsTeamTraining::GetMenuTitle()
{
	return menuTitle_;
}

// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
/*
void JSutsTeamTraining::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}
*/

// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool JSutsTeamTraining::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

// Return true if window should be interactive
bool JSutsTeamTraining::IsActiveOverlay()
{
	return true;
}

// Called when window is opened
void JSutsTeamTraining::OnOpen()
{
	isWindowOpen_ = true;
}

// Called when window is closed
void JSutsTeamTraining::OnClose()
{
	isWindowOpen_ = false;
}

