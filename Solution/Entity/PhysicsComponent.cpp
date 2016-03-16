#include "stdafx.h"
#include <PhysicsCallbackStruct.h>
#include "PhysicsComponent.h"
#include "PhysicsComponentData.h"
#include <PhysicsInterface.h>


PhysicsComponent::PhysicsComponent(Entity& aEntity, const PhysicsComponentData& aPhysicsComponentData
	, const std::string& aFBXPath)
	: Component(aEntity)
	, myData(aPhysicsComponentData)
{
	for (int i = 0; i < 16; ++i)
	{
		my4x4Float[i] = myEntity.GetOrientation().myMatrix[i];
		myThread4x4Float[i] = myEntity.GetOrientation().myMatrix[i];
	}

	myPosition[0] = my4x4Float[12];
	myPosition[1] = my4x4Float[13];
	myPosition[2] = my4x4Float[14];

	myPhysicsType = aPhysicsComponentData.myPhysicsType;

	bool shouldAddToPhysicsScene = true;
	if (myEntity.GetType() == eEntityType::EXPLOSION)
	{
		shouldAddToPhysicsScene = false;
	}

	if (myPhysicsType != ePhysics::CAPSULE)
	{
		myCallbackStruct = Prism::PhysicsCallbackStruct(myData, std::bind(&PhysicsComponent::SwapOrientations, this), std::bind(&PhysicsComponent::UpdateOrientation, this));
		Prism::PhysicsInterface::GetInstance()->Create(this, myCallbackStruct, my4x4Float, aFBXPath, &myDynamicBody, &myStaticBody, &myShapes, shouldAddToPhysicsScene);
	}
	else if (myPhysicsType == ePhysics::CAPSULE)
	{
		myCapsuleControllerId = Prism::PhysicsInterface::GetInstance()->CreatePlayerController(myEntity.GetOrientation().GetPos(), this);
	}
}


PhysicsComponent::~PhysicsComponent()
{
	delete[] myShapes;
	myShapes = nullptr;
}

void PhysicsComponent::Update(float)
{
	if (myPhysicsType == ePhysics::KINEMATIC)
	{
		Prism::PhysicsInterface::GetInstance()->MoveToPosition(myDynamicBody, myEntity.GetOrientation().GetPos());
	}
}

void PhysicsComponent::Reset()
{
	if (myDynamicBody != nullptr && myPhysicsType != ePhysics::KINEMATIC)
	{
		Prism::PhysicsInterface::GetInstance()->SetVelocity(myDynamicBody, CU::Vector3<float>(0, 0, 0));
	}
}

void PhysicsComponent::Sleep()
{
	if (myDynamicBody != nullptr)
	{
		Prism::PhysicsInterface::GetInstance()->Sleep(myDynamicBody);
	}
}

void PhysicsComponent::Wake()
{
	if (myDynamicBody != nullptr)
	{
		Prism::PhysicsInterface::GetInstance()->Wake(myDynamicBody);
	}
}

void PhysicsComponent::SwapOrientations()
{
	std::swap(my4x4Float, myThread4x4Float);
}

float* PhysicsComponent::GetPosition()
{
	DL_ASSERT("Not used? Not verified to work.");
	return &my4x4Float[12];
}

float* PhysicsComponent::GetOrientation()
{
	return my4x4Float;
}

void PhysicsComponent::UpdateOrientation()
{
	DL_ASSERT_EXP(myPhysicsType == ePhysics::DYNAMIC, "Cant update Orientation on STATIC PhysEntities");

	Prism::PhysicsInterface::GetInstance()->UpdateOrientation(myDynamicBody, myShapes, myThread4x4Float);


	myPosition[0] = myThread4x4Float[12];
	myPosition[1] = myThread4x4Float[13];
	myPosition[2] = myThread4x4Float[14];
}

void PhysicsComponent::AddForce(const CU::Vector3<float>& aDirection, float aMagnitude)
{
	DL_ASSERT_EXP(myPhysicsType == ePhysics::DYNAMIC, "Cant add Force to STATIC objects");

	Prism::PhysicsInterface::GetInstance()->AddForce(myDynamicBody, aDirection, aMagnitude);
}

void PhysicsComponent::SetVelocity(const CU::Vector3<float>& aVelocity)
{
	DL_ASSERT_EXP(myPhysicsType == ePhysics::DYNAMIC, "Cant add Force to STATIC objects");

	Prism::PhysicsInterface::GetInstance()->SetVelocity(myDynamicBody, aVelocity);
}

void PhysicsComponent::TeleportToPosition(const CU::Vector3<float>& aPosition)
{
	DL_ASSERT_EXP(myPhysicsType != ePhysics::STATIC, "Cant add Force to STATIC objects");

	if (myDynamicBody != nullptr)
	{
		Prism::PhysicsInterface::GetInstance()->TeleportToPosition(myDynamicBody, aPosition);
	}
	else if (myStaticBody != nullptr)
	{
		Prism::PhysicsInterface::GetInstance()->TeleportToPosition(myStaticBody, aPosition);
	}
}

void PhysicsComponent::MoveToPosition(const CU::Vector3<float>& aPosition)
{
	DL_ASSERT_EXP(myPhysicsType != ePhysics::STATIC, "Cant add Force to STATIC objects");

	Prism::PhysicsInterface::GetInstance()->MoveToPosition(myDynamicBody, aPosition);
}

void PhysicsComponent::SetPlayerCapsulePosition(const CU::Vector3<float>& aPosition)
{
	Prism::PhysicsInterface::GetInstance()->SetPosition(myCapsuleControllerId, aPosition);
}

void PhysicsComponent::AddToScene()
{
	if (myPhysicsType == ePhysics::DYNAMIC || myPhysicsType == ePhysics::KINEMATIC)
	{
		Prism::PhysicsInterface::GetInstance()->Add(myDynamicBody);
	}
	else
	{
		Prism::PhysicsInterface::GetInstance()->Add(myStaticBody);
	}
}

void PhysicsComponent::RemoveFromScene()
{
	if (myPhysicsType == ePhysics::DYNAMIC || myPhysicsType == ePhysics::KINEMATIC)
	{
		Prism::PhysicsInterface::GetInstance()->Remove(myDynamicBody, myData);
	}
	else
	{
		//DL_ASSERT("Can't remove static objects");
		Prism::PhysicsInterface::GetInstance()->Remove(myStaticBody, myData);
	}

}

