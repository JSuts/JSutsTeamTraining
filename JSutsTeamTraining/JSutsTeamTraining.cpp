#include "pch.h"
#include "JSutsTeamTraining.h"



BAKKESMOD_PLUGIN(JSutsTeamTraining, "J_Suts Team Training", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

/* bakkesmod managed cvar variables */
float snapshotInterval = 0.010f; // time (s) between updates
int captureTime = 3; // length of capture (s)
int maxCapture = int(captureTime / snapshotInterval); // length of captures (GameStates)
int maxReps = 40;



void JSutsTeamTraining::onLoad()
{
	_globalCvarManager = cvarManager;

	cvarManager->registerNotifier("js_training_pack", 
		std::bind(&JSutsTeamTraining::loadPack, this, std::placeholders::_1), 
		"Load training pack", 
		PERMISSION_ONLINE);

	cvarManager->registerNotifier("js_training_ex_pack",
		std::bind(&JSutsTeamTraining::executeTrainingPack, this),
		"Load training pack",
		PERMISSION_ONLINE);

	cvarManager->registerNotifier("js_training_make_drill", 
		std::bind(&JSutsTeamTraining::createDrillFromCurrentReplayPosition, this), 
		"Create and save drill to memory", 
		PERMISSION_REPLAY);

	cvarManager->registerNotifier("js_training_load_drill", 
		std::bind(&JSutsTeamTraining::loadDrill, this), 
		"Load drill saved in memory", 
		PERMISSION_ONLINE);

	cvarManager->registerNotifier("js_training_save_pack", 
		std::bind(&JSutsTeamTraining::savePack, this), 
		"Save pack in memory to filesystem", 
		PERMISSION_ONLINE);

	cvarManager->registerNotifier("js_training_stop_drill", [&](std::vector<std::string> args) {
		training = false;

	}, "", 0);		 

	cvarManager->registerNotifier("js_training_make_pack", [&](std::vector<std::string> args) {
		currentPack = TrainingPack();

		// TODO: REMOVE BELOW probably

		currentPack.packName = "firstPack";
		// currentPack.packDescription = "Testing the first training pack";

	}, "", 0);		
	
	cvarManager->registerNotifier("js_training_next_drill", [&](std::vector<std::string> args) {
		if (currentPack.drills.size() > 1) {
			std::rotate(currentPack.drills.begin(), currentPack.drills.begin() + 1, currentPack.drills.end());
			currentDrill = currentPack.drills.at(0);
		}
		else {
			cvarManager->log("Only one drill in this pack");
		}

	}, "", 0);	

	cvarManager->registerNotifier("js_training_add_drill_to_pack", [&](std::vector<std::string> args) {
		
		// TODO: ERROR CHECK

		currentPack.drills.push_back(currentDrill);

	}, "", 0);

	// HOOK EVENTS
	/*
	gameWrapper->HookEvent("Function GameEvent_TA.Countdown.StartCountdownTimer", [this](std::string eventName) {
		// check if in game with authority?

		if (!training)
			return;

		cvarManager->log("Start Countdown Timer");

		if (gameWrapper->IsInGame())
			cvarManager->log("in game");
		else {
			cvarManager->log("not in game");
			return;
		}


		ServerWrapper server = gameWrapper->GetCurrentGameState();
		if (server.HasAuthority())
			cvarManager->log("has authority");
		else {
			cvarManager->log("no authority");
			return;
		}

		cvarManager->getCvar("sv_soccar_gravity").setValue(-0.000001f);
		// placeActors();
		// currentDrill.place(server);
		// currentDrill.ball.apply(server.GetBall());


		// Teleport cars to location
		// Set gravity to zero
		
		});

	gameWrapper->HookEvent("Function GameEvent_TA.Countdown.OnGameStateTimeUpdated", [this](std::string eventName) {
		
		if (!training)
			return;

		// Same as startcountdown?
		// placeActors();
		// currentDrill.place(gameWrapper->GetGameEventAsServer());

		});
	
	gameWrapper->HookEvent("Function GameEvent_TA.Countdown.EndState", [this](std::string eventName) {

		if (!training)
			return;
		
		// DRILL TIME (if the safety precautions pass
		
		cvarManager->log("Countdown End State");

		cvarManager->getCvar("sv_soccar_gravity").setValue(-650);

		currentDrill.apply(gameWrapper->GetGameEventAsServer());

		cyclePlayers();

		// reset gravity to normal
		// teleport cars and apply again

		// AFTER A CERTAIN AMOUNT OF TIME, CHECK IF ANYONE IS NEAR THE BALL, AND IF NOT call OnDrillEnd()?

		});
	*/

	gameWrapper->HookEvent("Function TAGame.Ball_TA.OnHitGoal", [this](std::string eventName) {
		
		if (!training)
			return;

		loadDrill();
		/*
		auto server = gameWrapper->GetGameEventAsServer();
		currentDrill.place(server);
		// placeActors();
		gameWrapper->SetTimeout([&](GameWrapper* gw)
			{
				auto newServer = gw->GetGameEventAsServer();
				// newServer.SetCountdownTime(3); // works with at least StartCountDown
				newServer.StartCountDown(); // stops cars momentum, ball keeps going, starts countdown from 3, cars still affected by gravity, weird thing happens at base of walls, no momentum after countdown is over.
			}, .1);
		*/
		// check conditions

		// just call countdown??? or could also handle cycling

		// onDrillEnd();?

		/*
		if (!gameWrapper->IsInFreeplay() || rewindMode || !playingFromCheckpoint || !resetOnGoal) {
			return;
		}
		loadLatestCheckpoint();
		*/
		
		});


	//auto cvar = cvarManager->registerCvar("template_cvar", "hello-cvar", "just a example of a cvar");
	//auto cvar2 = cvarManager->registerCvar("template_cvar2", "0", "just a example of a cvar with more settings", true, true, -10, true, 10 );

	//cvar.addOnValueChanged([this](std::string cvarName, CVarWrapper newCvar) {
	//	cvarManager->log("the cvar with name: " + cvarName + " changed");
	//	cvarManager->log("the new value is:" + newCvar.getStringValue());
	//});

	//cvar2.addOnValueChanged(std::bind(&JSutsTeamTraining::YourPluginMethod, this, _1, _2));

	// enabled decleared in the header
	//enabled = std::make_shared<bool>(false);
	//cvarManager->registerCvar("TEMPLATE_Enabled", "0", "Enable the TEMPLATE plugin", true, true, 0, true, 1).bindTo(enabled);

	//cvarManager->registerNotifier("NOTIFIER", [this](std::vector<std::string> params){FUNCTION();}, "DESCRIPTION", PERMISSION_ALL);
	//cvarManager->registerCvar("CVAR", "DEFAULTVALUE", "DESCRIPTION", true, true, MINVAL, true, MAXVAL);//.bindTo(CVARVARIABLE);
	//gameWrapper->HookEvent("FUNCTIONNAME", std::bind(&TEMPLATE::FUNCTION, this));
	//gameWrapper->HookEventWithCallerPost<ActorWrapper>("FUNCTIONNAME", std::bind(&JSutsTeamTraining::FUNCTION, this, _1, _2, _3));
	//gameWrapper->RegisterDrawable(bind(&TEMPLATE::Render, this, std::placeholders::_1));


	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", [this](std::string eventName) {
	//	cvarManager->log("Your hook got called and the ball went POOF");
	//});
	// You could also use std::bind here
	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", std::bind(&JSutsTeamTraining::YourPluginMethod, this);
}

void JSutsTeamTraining::onUnload()
{
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
		
	gameWrapper->HookEvent("Function TAGame.Replay_TA.Tick", [this](std::string eventname) {

		// TODO: Error check? This is replay tick i suppose
		if (!record) {
			return;
		}

		cvarManager->log("size: " + std::to_string(currentDrill.history.size()));
		if (currentDrill.history.size() == maxCapture) { // Done recording
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
		if (elapsed < snapshotInterval) {  // last capture was too recent
			cvarManager->log("too soon");
			return;
		}

		lastCaptureTime = currentTime;


		currentDrill.history.emplace_back(replay);
		cvarManager->log("recording");

		});
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


	server.ResetPickups();
	training = true;

	gameWrapper->HookEvent("Function TAGame.RBActor_TA.PreAsyncTick", [this](std::string eventname) {
		
		auto server = gameWrapper->GetGameEventAsServer();

		// TODO: Probably error check in match and shit exists
		if (!training) {
			position = 0;
			gameWrapper->UnhookEvent("Function TAGame.RBActor_TA.PreAsyncTick");
			return;
		}


		if (position >= currentDrill.history.size()) { // Drill setup is complete
			// Send go message
			auto cars = server.GetCars();
			for (int i = 0; i < cars.Count(); i++) {
				server.SendGoMessage(cars.Get(i).GetPlayerController());
			}

			// Reset position, end hook, and cycle players for next drill
			position = 0;
			gameWrapper->UnhookEvent("Function TAGame.RBActor_TA.PreAsyncTick");
			cyclePlayers();
			return;
		}


		float currentTime = server.GetSecondsElapsed();
		float elapsed = currentTime - lastApplyTime;

		elapsed = std::min(elapsed, 0.03f);

		if (elapsed < 0) { // uncertain when this is the case
			lastApplyTime = currentTime;
			return;
		}
		if (elapsed < 0.01f) {  // last capture was too recent
			cvarManager->log("too soon");
			return;
		}

		lastApplyTime = currentTime;

		currentDrill.applyIndividual(server, position); // Set current position

		// Send countdown message when position is a mulltiple of (history.size / 3) (or 0)
		if ((position % (currentDrill.history.size() / 3)) == 0) { // position is some (x/3) of history.size
			int countdownNum = (3 - (position / (currentDrill.history.size() / 3))); // finds which third of history.size and subtracts from 3 for the countdown number
			auto cars = server.GetCars();
			for (int i = 0; i < cars.Count(); i++) {
				server.SendCountdownMessage(countdownNum, cars.Get(i).GetPlayerController());
			}
		}

		position++;
		});
}

void JSutsTeamTraining::executeTrainingPack()
{
	if (currentPack.drills.size() == 0)
	{
		cvarManager->log("No Drills");
		return;
	}

	if (currentPack.packName.empty())
		cvarManager->log("Executing current pack");
	else
		cvarManager->log("Executing " + currentPack.packName);

	// set current drill to first drill? or random drill based on preferences
	currentDrill = currentPack.drills[0];

	loadDrill();
}

void JSutsTeamTraining::loadPack(std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		// TODO: Log training packs
		cvarManager->log("Saved Training Packs");
		return;
	}

	std::filesystem::path path = gameWrapper->GetDataFolder() / args[1];

	cvarManager->log(path.string());

	// std::ifstream in(path, std::ios::binary);
	std::ifstream in(path, std::ios::binary);

	if (!in.good()) {
		cvarManager->log("Could not open file.");
		cvarManager->log("Trying to close file connection.");
		in.close();
		cvarManager->log(in.is_open() ? "open" : "closed");		
		return;
	}



	currentPack = TrainingPack(in);
	if (currentPack.drills.size() > 0) {
		currentPack.packName = args[1];
		cvarManager->log("num of drills: " + std::to_string(currentPack.drills.size()));
		currentDrill = currentPack.drills.at(0);
	}
	// cvarManager->log("loaded " + currentPack.packDescription);
	// cvarManager->log("loaded pack: " + currentPack.filepath);

	// check if in online game/able to execute drill, then execute training or just load
	cvarManager->log("Trying to close");
	in.close();
	cvarManager->log(in.is_open() ? "open" : "closed");

	loadDrill();
}

// TODO: Add savePackAs or some pack name change option
void JSutsTeamTraining::savePack() {
	std::filesystem::path path = gameWrapper->GetDataFolder() / currentPack.packName;
	cvarManager->log("Saving " + path.string());
	// std::ofstream out(gameWrapper->GetDataFolder() / (currentPack.packName + ".txt"), std::ios::binary | std::ios::out | std::ios::trunc);
	std::ofstream out(gameWrapper->GetDataFolder() / currentPack.packName, std::ios::binary | std::ios::out | std::ios::trunc);

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


