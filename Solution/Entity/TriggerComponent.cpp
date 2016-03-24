#include "stdafx.h"
#include "CollisionNote.h"
#include <GameStateMessage.h>
#include "PhysicsComponent.h"
#include <PostMaster.h>
#include <RespawnMessage.h>
#include "TriggerComponent.h"
#include "TriggerComponentData.h"

TriggerComponent::TriggerComponent(Entity& anEntity, const TriggerComponentData& someData)
	: Component(anEntity)
	, myData(someData)
	, myTriggerType(eTriggerType(someData.myTriggerType))
	, myHasTriggered(false)
	, myRespawnValue(-1)
	, myPlayersInside(0)
	, myRespawnTime(0.f)
{
}

TriggerComponent::~TriggerComponent()
{
}

void TriggerComponent::Update(float aDelta)
{
	if (myHasTriggered == true && myTriggerType == eTriggerType::RESPAWN)
	{
		myRespawnTime -= aDelta * myPlayersInside;

		if (myRespawnTime <= 0.f)
		{
			PostMaster::GetInstance()->SendMessage<RespawnMessage>(RespawnMessage(myRespawnValue));
			myEntity.GetComponent<PhysicsComponent>()->RemoveFromScene();
			myHasTriggered = false;
		}
	}
}

void TriggerComponent::ReceiveNote(const CollisionNote& aNote)
{
	if (aNote.myHasEntered == true)
	{
		++myPlayersInside;
		if (myTriggerType == eTriggerType::LEVEL_CHANGE)
		{
			PostMaster::GetInstance()->SendMessage(GameStateMessage(eGameState::LOAD_LEVEL, myData.myValue));
		}

		if (myData.myIsOneTime == true)
		{
			myEntity.GetComponent<PhysicsComponent>()->RemoveFromScene();
		}
	}
	else if (myTriggerType == eTriggerType::RESPAWN)
	{
		--myPlayersInside;
	}
}

void TriggerComponent::Activate()
{
	if (myTriggerType == eTriggerType::RESPAWN)
	{
		myRespawnTime = float(myData.myValue);
		myPlayersInside = 0;
		myHasTriggered = true;
	}
}

int TriggerComponent::GetValue() const
{
	return myData.myValue;
}

bool TriggerComponent::IsClientSide() const
{
	return myData.myIsClientSide;
}

bool TriggerComponent::GetIsActiveFromStart() const
{
	return myData.myIsActiveFromStart;
}