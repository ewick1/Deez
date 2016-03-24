#pragma once
#include <NetworkSubscriber.h>

namespace Prism
{
	class Camera;
}

struct NotificationText
{
	std::string myText;
	CU::Vector4<float> myColor;
	float myLifeTime;
	bool myIsActive;
};

struct MissionText
{
	std::string myText;
	CU::Vector4<float> myColor;
	int myMissionGID;
	bool myIsActive;
};

class TextEventManager : public NetworkSubscriber
{
public:
	TextEventManager(const Prism::Camera* aCamera);
	~TextEventManager();

	void Update(float aDeltaTime);
	void Render();

	void AddNotification(std::string aText, float aLifeTime = 5.f, CU::Vector4<float> aColor = { 1.f, 1.f, 1.f, 1.f });
	void ReceiveNetworkMessage(const NetMessageText& aMessage, const sockaddr_in& aSenderAddress) override;

private:

	const Prism::Camera* myCamera;

	CU::GrowingArray<NotificationText*> myNotifications;
	CU::GrowingArray<MissionText*> myMissionTexts;

	float myTextStartFadingTime;

};