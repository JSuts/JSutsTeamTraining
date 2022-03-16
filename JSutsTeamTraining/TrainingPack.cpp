#include "pch.h"
#include "JSutsTeamTraining.h"

/*
 * base64enc and base64dec from https://stackoverflow.com/a/34571089.  No license
 * information provided.
 */

const std::string_view b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64enc(const std::string in) {
	std::string out;

	unsigned val = 0;
	int valb = -6;
	for (unsigned char c : in) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(b64[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) out.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}

const std::string base64dec(const std::string in) {
	std::string out;

	std::vector<int> T(256, -1);
	for (int i = 0; i < 64; i++) T[b64[i]] = i;

	unsigned val = 0;
	int valb = -8;
	for (unsigned char c : in) {
		if (T[c] == -1) break;
		val = (val << 6) + T[c];
		valb += 6;
		if (valb >= 0) {
			out.push_back(char((val >> valb) & 0xFF));
			valb -= 8;
		}
	}
	return out;
}

/* Saving and Loading functions */
static inline void readVec(std::istream& in, Vector& v) {
	readPOD(in, v.X);
	readPOD(in, v.Y);
	readPOD(in, v.Z);
}
static inline void writeVec(std::ostream& out, const Vector& v) {
	writePOD(out, v.X);
	writePOD(out, v.Y);
	writePOD(out, v.Z);
}
static inline void readRot(std::istream& in, Rotator& r) {
	readPOD(in, r.Pitch);
	readPOD(in, r.Yaw);
	readPOD(in, r.Roll);
}
static inline void writeRot(std::ostream& out, const Rotator& r) {
	writePOD(out, r.Pitch);
	writePOD(out, r.Yaw);
	writePOD(out, r.Roll);
}


/* Mirroring Helpers (from https://github.com/bakkesmodorg/BakkesMod2-Plugins/blob/45d86ebb621addf6b9f0e10b2a8df0a0dcfb8f84/TrainingPlugin/TrainingPlugin.cpp) */
Vector mirror_it(Vector v, bool mir) {
	if (mir) {
		v.X = -v.X;
	}
	return v;
}

Rotator mirror_it(Rotator r, bool mir) {
	if (mir) {
		if (r.Yaw > 0) {
			if (r.Yaw > 16383) {
				r.Yaw = 16383 - (r.Yaw - 16383);
			}
			else {
				r.Yaw = 16383 + (16383 - r.Yaw);

			}
		}
		else {
			if (r.Yaw > -16383) {
				r.Yaw = -16383 - (16383 - abs(r.Yaw));
			}
			else {
				r.Yaw = -16383 + (abs(r.Yaw) - 16383);
			}
		}
	}
	return r;
}


/* ActorData Methods */

ActorData::ActorData()
{
	location = Vector(0, 0, 0);
	velocity = Vector(0, 0, 0);
	rotation = Rotator(0, 0, 0);
	angVelocity = Vector(0, 0, 0);
}

ActorData::ActorData(ActorWrapper actor) {
	location = actor.GetLocation();
	velocity = actor.GetVelocity();
	rotation = actor.GetRotation();
	angVelocity = actor.GetAngularVelocity();
}

ActorData::ActorData(std::istream& in) {
	readVec(in, location);
	readVec(in, velocity);
	readRot(in, rotation);
	readVec(in, angVelocity);
}

ActorData::ActorData(std::istringstream& stream) {
	readVec(stream, location);
	readVec(stream, velocity);
	readRot(stream, rotation);
	readVec(stream, angVelocity);
}

void ActorData::write(std::ostream& out) const {
	writeVec(out, location);
	writeVec(out, velocity);
	writeRot(out, rotation);
	writeVec(out, angVelocity);
}

void ActorData::apply(ActorWrapper actor, bool mirror) const {
		actor.SetLocation(mirror_it(location, mirror));
		actor.SetVelocity(mirror_it(velocity, mirror));
		actor.SetRotation(mirror_it(rotation, mirror));
		actor.SetAngularVelocity(mirror_it(angVelocity, mirror), false);
}

void ActorData::place(ActorWrapper actor, bool mirror) const {
	actor.SetLocation(mirror_it(location, mirror));
	actor.SetVelocity(0);
	actor.SetRotation(mirror_it(rotation, mirror));
	actor.SetAngularVelocity(0, false);
}



/* CarData Methods */
CarData::CarData() {
	// "Blank" cars get randomly placed on the ceiling to keep them out of the play (and randomized to keep it a little fun)
	actorData = ActorData();
	float X = std::rand() % (4096*2) - 4096; // Side walls are at X = +/-4096
	float Y = std::rand() % (5120 * 2) - 5120; // Back walls are at Y = +/-5120
	float Z = 2100; // Ceiling is at Z = 2044

	actorData.location = Vector(X, Y, Z);
	actorData.rotation.Yaw = std::rand() % 6 - 3;
	boostAmount = 99;
}

CarData::CarData(CarWrapper car) {
	actorData = ActorData(car);

	boostAmount = car.GetBoostComponent().IsNull() ? 0 : car.GetBoostComponent().GetCurrentBoostAmount();
	// Save last jump time only if the player jumped.
	// After applying this, we will remove the player's dodge when the jump timer expires.
	lastJumped = !car.GetbJumped() || car.GetJumpComponent().IsNull() ? -1 : car.GetJumpComponent().GetInactiveTime();
	hasDodge = !car.GetbDoubleJumped() && lastJumped < MAX_DODGE_TIME;
}

CarData::CarData(std::istream& in) {
	actorData = ActorData(in);
	readPOD(in, boostAmount);
	readPOD(in, hasDodge);
	readPOD(in, lastJumped);
	readPOD(in, identifier);
}

CarData::CarData(std::istringstream& stream) {
	actorData = ActorData(stream);
	readPOD(stream, boostAmount);
	readPOD(stream, hasDodge);
	readPOD(stream, lastJumped);
	readPOD(stream, identifier);

}

void CarData::write(std::ostream& out) const {
	actorData.write(out);
	writePOD(out, boostAmount);
	writePOD(out, hasDodge);
	writePOD(out, lastJumped);
	writePOD(out, identifier);
}

void CarData::apply(CarWrapper car, bool mirror) const {
	actorData.apply(car, mirror);
	if (!car.GetBoostComponent().IsNull()) {
		// DEBUG: Does this give a consistent amount to everyone
		// HACK: just give everyone besides me 2x boost
		car.GetBoostComponent().SetCurrentBoostAmount(boostAmount);
		car.GetBoostComponent().ClientGiveBoost(boostAmount); 
	}
	car.SetbDoubleJumped(!hasDodge);
	car.SetbJumped(!hasDodge);
}

void CarData::place(CarWrapper car, bool mirror) const {
	actorData.place(car, mirror);
	if (!car.GetBoostComponent().IsNull()) {
		// DEBUG: Does this give a consistent amount to everyone
		// HACK: just give everyone besides me 2x boost
		car.GetBoostComponent().SetCurrentBoostAmount(boostAmount);
		car.GetBoostComponent().ClientGiveBoost(boostAmount); 
	}
	car.SetbDoubleJumped(!hasDodge);
	car.SetbJumped(!hasDodge);
}



/* GameState Methods */
GameState::GameState() {
	ball = ActorData();
	cars.emplace_back();
}

GameState::GameState(ServerWrapper sw) {
	ball = ActorData(sw.GetBall());
	ArrayWrapper<CarWrapper> serverCars = sw.GetCars();
	for (int i = 0; i < serverCars.Count(); i++)
	{
		CarData car = CarData(serverCars.Get(i));
		car.identifier = i;
		// Add blue team to the front, add orange team to the end
		if (serverCars.Get(i).GetTeamNum2() == 0)
			cars.insert(cars.begin(), car);
		else
			cars.insert(cars.end(), car);
	}
}

GameState::GameState(std::istream& in) {
	ball = ActorData(in);

	int32_t size;
	readPOD(in, size);

	for (int32_t i = 0; i < size; i++)
	{
		cars.emplace_back(in);
	}
}

GameState::GameState(const std::string enc) {
	std::string dec = base64dec(enc);
	std::istringstream stream(dec);

	ball = ActorData(stream);

	int32_t size;
	readPOD(stream, size);

	for (int32_t i = 0; i < size; i++)
	{
		cars.emplace_back(stream);
	}
}

void GameState::write(std::ostream& out) const {
	ball.write(out);

	auto size = int32_t(cars.size());

	writePOD(out, size);

	for (auto& car : cars) {
		car.write(out);
	}
}

void GameState::apply(ServerWrapper sw, bool mirror) const {
	if (sw.GetBall().IsNull() || sw.GetCars().IsNull()) {
		return;
	}
	ball.apply(sw.GetBall(), mirror);
	ArrayWrapper<CarWrapper> serverCars = sw.GetCars();
	for (int i = 0; i < serverCars.Count(); i++)
	{
		if (i < cars.size())
			cars.at(i).apply(serverCars.Get(i), mirror);
		else
			CarData().apply(serverCars.Get(i), mirror); // This is for extra cars in the lobby that aren't apart of the drill
	}
}

void GameState::place(ServerWrapper sw, bool mirror) const {
	if (sw.GetBall().IsNull() || sw.GetCars().IsNull()) {
		return;
	}
	ball.place(sw.GetBall(), mirror);
	ArrayWrapper<CarWrapper> serverCars = sw.GetCars();
	for (int i = 0; i < serverCars.Count(); i++)
	{
		if (i < cars.size())
			cars.at(i).place(serverCars.Get(i), mirror);
		else
			CarData().place(serverCars.Get(i), mirror); // This is for extra cars in the lobby that aren't apart of the drill
	}
}

const std::string GameState::toString() const {
	std::ostringstream dec;
	write(dec);
	return base64enc(dec.str());
}



/* DrillData Methods */
DrillData::DrillData() {
	// history.emplace_back(GameState());
	history;
	fps = 100;
}

DrillData::DrillData(ServerWrapper sw) {
	history.emplace_back(GameState(sw));
	fps = 100;
}

DrillData::DrillData(std::istream& in) {
	readPOD(in, fps);

	int32_t size;
	readPOD(in, size);

	for (int32_t i = 0; i < size; i++)
	{
		history.emplace_back(in);
	}
}

DrillData::DrillData(const std::string enc) {
	std::string dec = base64dec(enc);
	std::istringstream stream(dec);

	readPOD(stream, fps);

	int32_t size;
	readPOD(stream, size);

	for (int32_t i = 0; i < size; i++)
	{
		history.emplace_back(stream);
	}
}

void DrillData::write(std::ostream& out) const {
	writePOD(out, fps);
	auto size = int32_t(history.size());

	writePOD(out, size);

	for (auto& gs : history) {
		gs.write(out);
	}
}

void DrillData::apply(ServerWrapper sw, bool mirror) const {
	if (sw.GetBall().IsNull() || sw.GetCars().IsNull()) {
		return;
	}
	for (auto& gs : history) {
		gs.apply(sw, mirror);
	}
}

void DrillData::applyIndividual(ServerWrapper sw, int index, bool mirror) const {
	if (sw.GetBall().IsNull() || sw.GetCars().IsNull()) {
		return;
	}
	if (index <= 0) // first index or less
		history.at(0).apply(sw, mirror); // apply first index
	else if (index >= history.size() - 1) // last index or more
		history.at(history.size() - 1).apply(sw, mirror); // apply last index
	else
		history.at(index).apply(sw, mirror);
}


void DrillData::place(ServerWrapper sw, bool mirror) const {
	if (sw.GetBall().IsNull() || sw.GetCars().IsNull()) {
		return;
	}
	for (auto& gs : history) {
		gs.place(sw, mirror);
	}
}

void DrillData::placeIndividual(ServerWrapper sw, int index, bool mirror) const {
	if (sw.GetBall().IsNull() || sw.GetCars().IsNull()) {
		return;
	}
	if (index <= 0) // first index or less
		history.at(0).place(sw, mirror); // apply first index
	else if (index >= history.size() - 1) // last index or more
		history.at(history.size() - 1).place(sw, mirror); // apply last index
	else
		history.at(index).place(sw, mirror);
}

const std::string DrillData::toString() const {
	std::ostringstream dec;
	write(dec);
	return base64enc(dec.str());
}

/* TrainingPack Methods */
TrainingPack::TrainingPack() {
	/*
	packName = "New Training Pack";
	packDescription = "";
	filepath = "";
	*/
}

TrainingPack::TrainingPack(std::ifstream& in)
{
	int32_t version;

	readPOD(in, version);

	if (version != FILE_VERSION) { // file does not match current version, can not read
		_globalCvarManager->log("File version mismatch");
		return;
	}
	
	int32_t size;
	readPOD(in, size);

	for (int32_t i = 0; i < size; i++)
	{
		drills.emplace_back(in);
	}
}

void TrainingPack::write(std::ostream& out) const
{
	writePOD(out, FILE_VERSION);

	auto size = int32_t(drills.size());
	writePOD(out, size);
	for (auto& drill : drills) {
		drill.write(out);
	}
}

const std::string TrainingPack::toString() const {
	std::ostringstream dec;
	write(dec);
	return base64enc(dec.str());
}