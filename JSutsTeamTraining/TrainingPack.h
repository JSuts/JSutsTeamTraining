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
	void apply(ActorWrapper actor) const;
	void place(ActorWrapper actor) const;
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
	void apply(CarWrapper c) const;
	void place(CarWrapper c) const;
	
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
	void apply(ServerWrapper sw) const;
	void place(ServerWrapper sw) const;

	const std::string toString() const;
};


class DrillData {
public:	
	std::vector<GameState> history;

	DrillData();
	DrillData(ServerWrapper sw);
	// DrillData(CarWrapper cw, BallWrapper bw); // not needed?
	DrillData(std::istream& in);
	DrillData(std::string enc);

	void write(std::ostream& out) const;
	void apply(ServerWrapper sw) const;
	void applyIndividual(ServerWrapper sw, int index) const;
	void place(ServerWrapper sw) const;
	void placeIndividual(ServerWrapper sw, int index) const;

	const std::string toString() const;
};

class TrainingPack {
public:
	std::vector<DrillData> drills;
	std::string packName;
	// std::string packDescription;
	// std::string filepath;

	TrainingPack();
	TrainingPack(std::ifstream& in);

	void write(std::ostream& out) const;

	const std::string toString() const;
};
/*
class TrainingPack
{
public:
	struct CarData {
		Vector CarLocation, CarVelocity;
		Rotator CarRotation;
		float Boost;
		std::string OriginalPlayerName;
		//flip indicator?

		void setCar(CarWrapper gameCar);
		void placeCar(CarWrapper gameCar); // Freeze Car at location
	};
	struct DrillData {
		Vector BallLocation, BallVelocity, BallAngularVelocity;
		Rotator BallRotation;
		std::string ShotName;
		std::vector<CarData> Cars;

		void setBall(BallWrapper gameBall); // Set location and velocities
		void placeBall(BallWrapper gameBall); // Freeze ball at location
	};	

	TrainingPack();
	TrainingPack(std::string filepath);

	// Either add another method to allow passing all three options, or combine into new method with three
	// void setName(std::string name);
	// void setDescription(std::string description);
	// void setFilepath(std::string filepath);
	
	void save();
	void addDrill(DrillData drill);

//private:
	std::vector<DrillData> Drills;
	std::string PackName;
	std::string PackDescription;
	std::string filepath;
};
*/

