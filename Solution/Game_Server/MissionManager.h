#pragma once
#include <GameEnum.h>
#include <unordered_map>
class Mission;
class MissionManager : public Subscriber
{
public:
	MissionManager(const std::string& aMissionXMLPath);
	~MissionManager();

	void Update(float aDeltaTime);

	void SetMission(int aId);
	eMissionType GetCurrentMissionType() const;

	void ReceiveMessage(const EnemyKilledMessage& aMessage) override;
	void ReceiveMessage(const DefendTouchMessage& aMessage) override;

private:
	eActionEventType GetType(const std::string& aType);
	void LoadMissions(const std::string& aMissionXMLPath);
	Mission* myCurrentMission;
	std::unordered_map<int, Mission*> myMissions;
};