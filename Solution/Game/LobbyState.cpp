#include "stdafx.h"

#include <AudioInterface.h>
#include "ClientNetworkManager.h"
#include <Cursor.h>
#include <fstream>
#include <GUIManager.h>
#include "InGameState.h"
#include <InputWrapper.h>
#include "LobbyState.h"
#include <ModelLoader.h>
#include <NetMessageDisconnect.h>
#include <NetMessageLoadLevel.h>
#include <NetMessageSetLevel.h>
#include <NetMessageRequestStartLevel.h>
#include <OnClickMessage.h>
#include <OnRadioButtonMessage.h>
#include <PostMaster.h>
#include <SharedNetworkManager.h>
#include <TextProxy.h>

LobbyState::LobbyState()
	: myGUIManager(nullptr)
	, myGUIManagerHost(nullptr)
	, myLevelToStart(-1)
	, myStartGame(false)
	, myServerLevelHash(0)
	, myRefreshPlayerListTimer(0)
{
}


LobbyState::~LobbyState()
{
	SAFE_DELETE(myGUIManager);
	SAFE_DELETE(myGUIManagerHost);
	SAFE_DELETE(myText);
	SAFE_DELETE(myPlayerListText);
	SAFE_DELETE(myWaitingForHostText);
	myCursor = nullptr;
	PostMaster::GetInstance()->UnSubscribe(eMessageType::ON_CLICK, this);
	PostMaster::GetInstance()->UnSubscribe(eMessageType::ON_RADIO_BUTTON, this);
	SharedNetworkManager::GetInstance()->UnSubscribe(eNetMessageType::ON_DISCONNECT, this);
	SharedNetworkManager::GetInstance()->UnSubscribe(eNetMessageType::SET_LEVEL, this);
	SharedNetworkManager::GetInstance()->UnSubscribe(eNetMessageType::LOAD_LEVEL, this);
}

void LobbyState::InitState(StateStackProxy* aStateStackProxy, GUI::Cursor* aCursor)
{
	PostMaster::GetInstance()->Subscribe(eMessageType::ON_CLICK, this);
	PostMaster::GetInstance()->Subscribe(eMessageType::ON_RADIO_BUTTON, this);
	SharedNetworkManager::GetInstance()->Subscribe(eNetMessageType::ON_DISCONNECT, this);
	SharedNetworkManager::GetInstance()->Subscribe(eNetMessageType::SET_LEVEL, this);
	SharedNetworkManager::GetInstance()->Subscribe(eNetMessageType::LOAD_LEVEL, this);
	myCursor = aCursor;
	myIsActiveState = true;
	myIsLetThrough = false;
	myStateStatus = eStateStatus::eKeepState;
	myStateStack = aStateStackProxy;

	myGUIManager = new GUI::GUIManager(myCursor, "Data/Resource/GUI/GUI_Lobby.xml", nullptr, -1);
	myGUIManagerHost = new GUI::GUIManager(myCursor, "Data/Resource/GUI/GUI_Lobby_host.xml", nullptr, -1);

	myText = Prism::ModelLoader::GetInstance()->LoadText(Prism::Engine::GetInstance()->GetFont(Prism::eFont::CONSOLE));
	myPlayerListText = Prism::ModelLoader::GetInstance()->LoadText(Prism::Engine::GetInstance()->GetFont(Prism::eFont::CONSOLE));
	myWaitingForHostText = Prism::ModelLoader::GetInstance()->LoadText(Prism::Engine::GetInstance()->GetFont(Prism::eFont::CONSOLE));
	Prism::ModelLoader::GetInstance()->WaitUntilFinished();


	myText->SetPosition({ 800.f, 800.f });
	myText->SetText("No current level selected.");
	myText->SetScale({ 1.f, 1.f });

	myPlayerListText->SetPosition({ 20, 800.f });
	myPlayerListText->SetText("Player Online:\n");
	myPlayerListText->SetScale({ 1.f, 1.f });

	myWaitingForHostText->SetPosition({ 800.f, 200.f });
	myWaitingForHostText->SetText("Waiting for host...");
	myWaitingForHostText->SetScale({ 1.f, 1.f });

	const CU::Vector2<int>& windowSize = Prism::Engine::GetInstance()->GetWindowSizeInt();
	OnResize(windowSize.x, windowSize.y);

	myCursor->SetShouldRender(true);
	myLevelToStart = -1;

	Prism::Audio::AudioInterface::GetInstance()->PostEvent("Stop_MainMenu", 0);
}

void LobbyState::EndState()
{
	myIsActiveState = false;
	myCursor->SetShouldRender(false);
	myLevelToStart = -1;
}

void LobbyState::OnResize(int aX, int aY)
{
	myGUIManager->OnResize(aX, aY);
}

const eStateStatus LobbyState::Update(const float& aDeltaTime)
{
	if (CU::InputWrapper::GetInstance()->KeyDown(DIK_ESCAPE) == true
		|| CU::InputWrapper::GetInstance()->KeyDown(DIK_N) == true)
	{
		ClientNetworkManager::GetInstance()->AddMessage(NetMessageDisconnect(ClientNetworkManager::GetInstance()->GetGID()));
		return eStateStatus::ePopMainState;
	}

	myRefreshPlayerListTimer -= aDeltaTime;
	if (myRefreshPlayerListTimer <= 0)
	{
		myRefreshPlayerListTimer = 1.f;
		std::string playerListText = "Players Online:\n";
		playerListText += ClientNetworkManager::GetInstance()->GetName() + "\n";
		const CU::GrowingArray<OtherClients>& playerList = ClientNetworkManager::GetInstance()->GetClients();
		for each (const OtherClients& player in playerList)
		{
			playerListText += player.myName + "\n";
		}
		myPlayerListText->SetText(playerListText);
	}

	ClientNetworkManager::GetInstance()->DebugPrint();

	if (CU::InputWrapper::GetInstance()->KeyDown(DIK_SPACE) == true)
	{
		ClientNetworkManager::GetInstance()->AddMessage(NetMessageRequestStartLevel());
	}

	if (myStartGame == true)
	{
		DL_ASSERT_EXP(myLevelToStart != -1, "Can't start level -1.");
		PostMaster::GetInstance()->UnSubscribe(eMessageType::ON_CLICK, this);
		PostMaster::GetInstance()->UnSubscribe(eMessageType::ON_RADIO_BUTTON, this);
		SharedNetworkManager::GetInstance()->UnSubscribe(eNetMessageType::ON_DISCONNECT, this);
		SharedNetworkManager::GetInstance()->UnSubscribe(eNetMessageType::SET_LEVEL, this);
		SharedNetworkManager::GetInstance()->UnSubscribe(eNetMessageType::LOAD_LEVEL, this);

		SET_RUNTIME(false);
		myStateStack->PushSubGameState(new InGameState(myLevelToStart, myServerLevelHash));
	}

	if (ClientNetworkManager::GetInstance()->GetGID() == 1)
	{
		myGUIManagerHost->Update(aDeltaTime);
	}
	else
	{
		myGUIManager->Update(aDeltaTime);
	}

	return myStateStatus;
}

void LobbyState::Render()
{
	if (ClientNetworkManager::GetInstance()->GetGID() == 1)
	{
		myGUIManagerHost->Render();
		myPlayerListText->Render();
	}
	else
	{
		myGUIManager->Render();
		myPlayerListText->Render();
		myWaitingForHostText->Render();
		myText->Render();
	}

	DEBUG_PRINT(myLevelToStart+1);
}

void LobbyState::ResumeState()
{
	myStartGame = false;
	myIsActiveState = true;
	myLevelToStart = -1;
}

void LobbyState::ReceiveMessage(const OnClickMessage& aMessage)
{
	if (myIsActiveState == true)
	{
		switch (aMessage.myEvent)
		{
		case eOnClickEvent::START_GAME:
			ClientNetworkManager::GetInstance()->AddMessage(NetMessageRequestStartLevel());
			break;
		case eOnClickEvent::GAME_QUIT:
			Prism::Audio::AudioInterface::GetInstance()->PostEvent("Stop_AllElevators", 0);
			ClientNetworkManager::GetInstance()->AddMessage(NetMessageDisconnect(ClientNetworkManager::GetInstance()->GetGID()));
			myStateStatus = eStateStatus::ePopMainState;
			break;
		default:
			DL_ASSERT("Unknown event.");
			break;
		}
	}
}

void LobbyState::ReceiveMessage(const OnRadioButtonMessage& aMessage)
{
	DL_ASSERT_EXP(aMessage.myEvent == eOnRadioButtonEvent::LEVEL_SELECT, "Only level select in lobby state.");

	
	//int levelMusic = aMessage.myID;
	//levelMusic = min(levelMusic, 2);
	//std::string musicEvent("Play_ElevatorToLevel" + std::to_string(levelMusic));
	//Prism::Audio::AudioInterface::GetInstance()->PostEvent(musicEvent.c_str(), 0);

	ClientNetworkManager::GetInstance()->AddMessage(NetMessageSetLevel(aMessage.myID));
}

void LobbyState::ReceiveNetworkMessage(const NetMessageDisconnect& aMessage, const sockaddr_in&)
{
	if (aMessage.myClientID == ClientNetworkManager::GetInstance()->GetGID() || ClientNetworkManager::GetInstance()->GetGID() == 0)
	{
		myStateStatus = eStateStatus::ePopMainState;
	}
}

void LobbyState::ReceiveNetworkMessage(const NetMessageSetLevel& aMessage, const sockaddr_in&)
{
	Prism::Audio::AudioInterface::GetInstance()->PostEvent("Stop_AllElevators", 0);

	int levelMusic = aMessage.myLevelID;
	levelMusic = min(levelMusic, 2);
	std::string musicEvent("Play_ElevatorToLevel" + std::to_string(levelMusic));
	Prism::Audio::AudioInterface::GetInstance()->PostEvent(musicEvent.c_str(), 0);

	myLevelToStart = aMessage.myLevelID;
	myText->SetText(CU::Concatenate("Current level: %i", min(myLevelToStart+1, 3)));
}

void LobbyState::ReceiveNetworkMessage(const NetMessageLoadLevel& aMessage, const sockaddr_in&)
{
	myLevelToStart = aMessage.myLevelID;
	myServerLevelHash = aMessage.myLevelHash;

	myStartGame = true;
}
