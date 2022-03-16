#include "pch.h"
#include "JSutsTeamTraining.h"



BAKKESMOD_PLUGIN(JSutsTeamTraining, "J_Suts Team Training", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

/* bakkesmod managed cvar variables */
std::filesystem::path folderPath; // location of plugin files
float snapshotInterval = 0.010f; // time (s) between updates
int captureTime = 3; // length of capture (s)
int maxCapture = 300; // length of captures (GameStates)
int maxReps = 40;
bool mirrorHalfway = true;
bool manualCaptureSettings = false;
bool overridePlaybackSettings = false;

void JSutsTeamTraining::onLoad()
{
	_globalCvarManager = cvarManager;

	// This should somehow take into account previous settings. Also, do the addOnValueChanged functions below need to change as well?
	int fps = gameWrapper->GetSettings().GetVideoSettings().MaxFPS;
	maxCapture = captureTime * fps;
	snapshotInterval = (float) captureTime / (float) maxCapture;

	folderPath = gameWrapper->GetDataFolder() / "JSutsTeamTraining";

	if (!std::filesystem::exists(folderPath)) {
		cvarManager->log(folderPath.string() + " does not exist.");
		if (std::filesystem::create_directory(folderPath)) {
			cvarManager->log("Created " + folderPath.string());
		}
		else {
			cvarManager->log("Failed to create " + folderPath.string());
		}
	}
	else {
		cvarManager->log(folderPath.string() + " exists.");
	}
	

	cvarManager->registerCvar("js_training_var_max_reps", std::to_string(maxReps), "How many times to repeat a drill", true, true, 1, false)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
			maxReps = cvar.getIntValue();
		});
	cvarManager->registerCvar("js_training_var_snapshot_interval", std::to_string(snapshotInterval), "Minimum amount of time between recording frames", true)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
			snapshotInterval = cvar.getFloatValue();
		});
	cvarManager->registerCvar("js_training_var_capture_time", std::to_string(captureTime), "Approximate amount of \"time\" to record", true)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
			captureTime = cvar.getIntValue();
		});
	cvarManager->registerCvar("js_training_var_max_capture", std::to_string(maxCapture), "Number of \"frames\" to record", true, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
			maxCapture = cvar.getIntValue();
		});
	cvarManager->registerCvar("js_training_var_mirror_halfway", std::to_string(mirrorHalfway), "Mirrors the drill halfway through when enabled", true, true, false, true, true)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
			mirrorHalfway = cvar.getBoolValue();
		});
	cvarManager->registerCvar("js_training_var_manual_capture_settings", std::to_string(manualCaptureSettings), "Enable this to use your own capture parameters", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
			manualCaptureSettings = cvar.getBoolValue();
		});
	cvarManager->registerCvar("js_training_var_manual_capture_settings", std::to_string(manualCaptureSettings), "Enable this to use your own capture parameters", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
			manualCaptureSettings = cvar.getBoolValue();
		});
	cvarManager->registerCvar("js_training_var_override_playback_settings", std::to_string(overridePlaybackSettings), "Enable this to run drills at any parameters", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
		overridePlaybackSettings = cvar.getBoolValue();
		});		

	cvarManager->registerNotifier("js_training_pack", std::bind(&JSutsTeamTraining::loadPack, this, std::placeholders::_1), "Load training pack", PERMISSION_ONLINE);
	cvarManager->registerNotifier("js_training_load_drill", std::bind(&JSutsTeamTraining::loadDrill, this), "Load drill saved in memory", PERMISSION_ONLINE);
	cvarManager->registerNotifier("js_training_make_drill", std::bind(&JSutsTeamTraining::createDrillFromCurrentReplayPosition, this), "Create and save drill to memory", PERMISSION_REPLAY);
	cvarManager->registerNotifier("js_training_save_pack", std::bind(&JSutsTeamTraining::savePack, this), "Save pack in memory to filesystem", PERMISSION_ONLINE);

	cvarManager->registerNotifier("js_training_stop_drill", [&](std::vector<std::string> args) {
		training = false;
	}, "", 0);		 

	cvarManager->registerNotifier("js_training_make_pack", [&](std::vector<std::string> args) {
		currentPack = TrainingPack();
		// TODO: REMOVE BELOW probably
		currentPack.packName = "firstPack";
	}, "", 0);		
	
	cvarManager->registerNotifier("js_training_next_drill", [&](std::vector<std::string> args) {
		cycleDrills();
		loadDrill();
	}, "", 0);	

	cvarManager->registerNotifier("js_training_add_drill_to_pack", [&](std::vector<std::string> args) {
		// TODO: ERROR CHECK
		currentPack.drills.push_back(currentDrill);
	}, "", 0);


	// HOOK EVENTS
	gameWrapper->HookEvent("Function TAGame.Ball_TA.OnHitGoal", [this](std::string eventName) {
		if (!training)
			return;
		loadDrill();	
		});
}

void JSutsTeamTraining::onUnload()
{
	gameWrapper->UnhookEvent("Function TAGame.Ball_TA.OnHitGoal"); // resetDrillHook
	gameWrapper->UnhookEvent("Function TAGame.Replay_TA.Tick"); // createDrillHook
	gameWrapper->UnhookEvent("Function TAGame.RBActor_TA.PreAsyncTick"); // setupDrillHook
}

/**
Get's all ball and car information in current replay position.
Assigns to global "drill" variable.
*/
void JSutsTeamTraining::createDrillFromCurrentReplayPosition()
{
	/* Error Checks */
	if (!gameWrapper->IsInReplay()) {
		cvarManager->log("Must be in replay to create drill.");
		return;
	}

	ReplayServerWrapper replay = gameWrapper->GetGameEventAsReplay();
	if (!replay) {
		cvarManager->log("Current game state can not be found...");
		return;
	}

	cvarManager->log("unpause to capture upcoming moments");
	record = true;
	currentDrill = DrillData(); // or DrilData(replay);

	// TODO: check if uncapped frames and set to 100 if true, otherwise set to MaxFPS
	int fps = gameWrapper->GetSettings().GetVideoSettings().MaxFPS;
	if (fps > 360)
		fps = 100;

	currentDrill.fps = fps;

	if (!manualCaptureSettings) {
		maxCapture = captureTime * fps;
		snapshotInterval = (float)captureTime / (float)maxCapture;

		cvarManager->log("maxCapture = " + std::to_string(maxCapture));
		cvarManager->log("snapshotInterval = " + std::to_string(snapshotInterval));
	}
	gameWrapper->HookEvent("Function TAGame.Replay_TA.Tick", std::bind(&JSutsTeamTraining::createDrillHook, this, std::placeholders::_1));
}

void JSutsTeamTraining::loadDrill()
{
	// need way of checking if currentDrill is empty/null?
	if (!gameWrapper->IsInGame())
	{
		cvarManager->log("Need to be in game to load a drill.");
		return;
	}

	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server.HasAuthority())
	{
		cvarManager->log("Need to have authority to manipulate the lobby.");
		return;
	}

	BallWrapper ball = server.GetBall();
	if (ball.IsNull())
	{
		ball = server.SpawnBall(currentDrill.history.at(0).ball.location, 0, 0);
	}

	if (!overridePlaybackSettings) {
		int fps = gameWrapper->GetSettings().GetVideoSettings().MaxFPS;
		if (fps != currentDrill.fps) {
			cvarManager->log("This drill was recorded at " + std::to_string(currentDrill.fps) + " fps. Please set your fps to match.");
			return;
		}
		
		maxCapture = captureTime * fps;
		snapshotInterval = (float)captureTime / (float)maxCapture; // maybe could replace maxCapture with just the calculation, as maxCapture isn't used in the drill setup hook

		cvarManager->log("maxCapture = " + std::to_string(maxCapture));
		cvarManager->log("snapshotInterval = " + std::to_string(snapshotInterval));
	}
	server.ResetPickups();
	training = true;

	cvarManager->log("maxReps = " + std::to_string(maxReps));
	gameWrapper->HookEvent("Function TAGame.RBActor_TA.PreAsyncTick", std::bind(&JSutsTeamTraining::setupDrillHook, this, std::placeholders::_1));
}

void JSutsTeamTraining::loadPack(std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		// TODO: Log training packs
		cvarManager->log("Saved Training Packs: ");
		for (const auto& entry : std::filesystem::directory_iterator(folderPath))
			cvarManager->log(entry.path().filename().string());
		cvarManager->log("Alternatively, you may provide an absolute path to the file you're looking for.");
		return;
	}

	std::filesystem::path path = folderPath / args[1];

	cvarManager->log(path.string());

	// std::ifstream in(path, std::ios::binary);
	std::ifstream in(path, std::ios::binary);

	if (!in.good()) {
		cvarManager->log("Could not open file.");
		cvarManager->log("Trying to close file connection.");
		in.close();
		cvarManager->log(in.is_open() ? "open" : "closed");
		
		// Check if input is an entire path itself
		path = std::filesystem::path(args[1]);
		cvarManager->log(path.string());
		std::ifstream in(path, std::ios::binary);
		
		// This doesn't quite work how I expect. It works for just copying and pasting an entire path, 
		// but doesn't work with relative pathing, even though it seems like the standard bakkesmod path is relative

		if (!in.good()) {
			cvarManager->log("Could not open location.");
			cvarManager->log("Trying to close file connection.");
			in.close();
			cvarManager->log(in.is_open() ? "open" : "closed");
			return;
		}
	}



	currentPack = TrainingPack(in);
	if (currentPack.drills.size() > 0) {
		currentPack.packName = args[1];
		cvarManager->log("num of drills: " + std::to_string(currentPack.drills.size()));
		currentDrill = currentPack.drills.at(0);
		loadDrill();
	}
	
	cvarManager->log("Trying to close");
	in.close();
	cvarManager->log(in.is_open() ? "open" : "closed");
}

// TODO: Add savePackAs or some pack name change option
void JSutsTeamTraining::savePack() {
	std::filesystem::path path = folderPath / currentPack.packName;
	cvarManager->log("Saving " + path.string());
	// std::ofstream out(gameWrapper->GetDataFolder() / (currentPack.packName + ".txt"), std::ios::binary | std::ios::out | std::ios::trunc);
	std::ofstream out(path, std::ios::binary | std::ios::out | std::ios::trunc);

	currentPack.write(out);
	cvarManager->log("Trying to close");
	out.close();
	cvarManager->log(out.is_open() ? "open" : "closed");
}

void JSutsTeamTraining::cyclePlayers()
{
	// Rotate every car position... maybe not the best method
	for (int i = 0; i < currentDrill.history.size(); i++) {
		std::rotate(currentDrill.history.at(i).cars.begin(), currentDrill.history.at(i).cars.begin() + 1, currentDrill.history.at(i).cars.end());
	}
}

void JSutsTeamTraining::cycleDrills() {
	if (currentPack.drills.size() > 1) {
		std::rotate(currentPack.drills.begin(), currentPack.drills.begin() + 1, currentPack.drills.end());
		currentDrill = currentPack.drills.at(0);
		rep = 1; // Restart rep counter	
	}
	else {
		cvarManager->log("Only one drill in this pack");
	}
}

/* Hook bound methods */
// Bound to execute on "Function TAGame.RBActor_TA.PreAsyncTick" until all drill history is loaded
void JSutsTeamTraining::setupDrillHook(std::string eventname) {
	auto server = gameWrapper->GetGameEventAsServer();

	// TODO: Probably error check in match and shit exists
	// training may be turned off by another function
	if (!training) {
		position = 0;
		gameWrapper->UnhookEvent("Function TAGame.RBActor_TA.PreAsyncTick");
		return;
	}

	// Check if drill setup is complete
	if (position >= currentDrill.history.size()) { 
		// Send go message
		auto cars = server.GetCars();
		for (int i = 0; i < cars.Count(); i++) {
			server.SendGoMessage(cars.Get(i).GetPlayerController());
		}

		// Reset position, end hook, and cycle players for next drill
		position = 0;
		gameWrapper->UnhookEvent("Function TAGame.RBActor_TA.PreAsyncTick");
		cyclePlayers();

		// Check if desired number of reps have been completed
		rep++;
		if (rep > maxReps) {
			cycleDrills();
		}

		if (mirrorHalfway && rep > maxReps / 2)
			mirror = true;
		else
			mirror = false;

		return;
	}


	float currentTime = server.GetSecondsElapsed();
	float elapsed = currentTime - lastApplyTime;
	// elapsed = std::min(elapsed, 0.03f); // uncertain if this is necessary

	cvarManager->log("elapsed: " + std::to_string(elapsed));

	if (elapsed < 0) { // uncertain when this is the case
		lastApplyTime = currentTime;
		return;
	}
	if (elapsed < (snapshotInterval - (snapshotInterval * .01))) {  // last capture was too recent
		cvarManager->log("too soon " + std::to_string(position));
		return;
	}
		
	// Alert messages
	// Send countdown message when position is a mulltiple of (history.size / 3) (or 0)
	if ((position % (currentDrill.history.size() / 3)) == 0) { // position is some (x/3) of history.size
		int countdownNum = (3 - (position / (currentDrill.history.size() / 3))); // finds which third of history.size and subtracts from 3 for the countdown number
		auto cars = server.GetCars();
		for (int i = 0; i < cars.Count(); i++) {
			server.SendCountdownMessage(countdownNum, cars.Get(i).GetPlayerController());
		}
	}

	// Set drill 
	lastApplyTime = currentTime;
	currentDrill.applyIndividual(server, position, mirror); // Set current position
	position++;	
}

// Bound to execute on "Function TAGame.Replay_TA.Tick" until all drill history is recorded
void JSutsTeamTraining::createDrillHook(std::string eventname) {
	// TODO: Error check? This is replay tick i suppose
	if (!record) {
		return;
	}

	cvarManager->log("size: " + std::to_string(currentDrill.history.size()));
	if (currentDrill.history.size() >= maxCapture) { // Done recording
		record = false;
		cvarManager->log("Done recording");
		gameWrapper->UnhookEvent("Function TAGame.Replay_TA.Tick");
		return;
	}

	auto replay = gameWrapper->GetGameEventAsReplay();

	float currentTime = replay.GetSecondsElapsed();
	float elapsed = currentTime - lastCaptureTime;

	cvarManager->log("elapsed: " + std::to_string(elapsed));

	if (elapsed < 0) { // uncertain when this is the case
		elapsed = snapshotInterval;
	}
	if (elapsed < (snapshotInterval - (snapshotInterval * .01))) {  // last capture was too recent
		cvarManager->log("too soon");
		return;
	}

	lastCaptureTime = currentTime;

	currentDrill.history.emplace_back(replay);
	cvarManager->log("recording");
}


