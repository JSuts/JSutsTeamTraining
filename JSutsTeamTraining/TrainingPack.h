#pragma once

class ActorData {
public:
	Vector location, velocity, angVelocity;
	Rotator rotation;

	ActorData();
	ActorData(ActorWrapper actor);
	ActorData(std::istream& in);
	ActorData(std::istringstream& stream);

	void write(std::ostream& out) const;
	void apply(ActorWrapper actor, bool mirror = false) const;
	void place(ActorWrapper actor, bool mirror = false) const;
};

class CarData {
public:
	ActorData actorData;
	// ControllerInput input;
	float boostAmount = 100;
	bool hasDodge = true;
	bool bot = false;
	float lastJumped = 0;
	int32_t identifier = -1;
	// std::string originalPlayerName;

	CarData();
	CarData(CarWrapper car);
	CarData(std::istream& in);
	CarData(std::istringstream& stream);

	void write(std::ostream& out) const;
	void apply(CarWrapper c, bool mirror = false) const;
	void place(CarWrapper c, bool mirror = false) const;
	
};

class GameState {
public:
	ActorData ball;
	std::vector<CarData> cars;

	GameState();
	GameState(ServerWrapper sw);
	GameState(std::istream& in);
	GameState(std::string enc);

	void write(std::ostream& out) const;
	void apply(ServerWrapper sw, bool mirror = false) const;
	void place(ServerWrapper sw, bool mirror = false) const;

	const std::string toString() const;
};


class DrillData {
public:	
	std::vector<GameState> history;
	int32_t fps;

	DrillData();
	DrillData(ServerWrapper sw);
	// DrillData(CarWrapper cw, BallWrapper bw); // not needed?
	DrillData(std::istream& in);
	DrillData(std::string enc);

	void write(std::ostream& out) const;
	void apply(ServerWrapper sw, bool mirror = false) const;
	void applyIndividual(ServerWrapper sw, int index, bool mirror = false) const;
	void place(ServerWrapper sw, bool mirror = false) const;
	void placeIndividual(ServerWrapper sw, int index, bool mirror = false) const;

	const std::string toString() const;
};

class TrainingPack {
public:
	std::vector<DrillData> drills;
	std::string packName;

	TrainingPack();
	TrainingPack(std::ifstream& in);

	void write(std::ostream& out) const;

	const std::string toString() const;
};
