#include "stdafx.h"

#include "DamageNote.h"
#include "Entity.h"
#include <EmitterMessage.h>
#include <PostMaster.h>
#include <PhysEntity.h>
#include <PhysicsInterface.h>
#include "Shotgun.h"
#include <XMLReader.h>

Shotgun::Shotgun()
	:Weapon(eWeaponType::SHOTGUN)
{
	XMLReader reader;
	reader.OpenDocument("Data/Setting/SET_weapons.xml");
	tinyxml2::XMLElement* root = reader.ForceFindFirstChild("root");

	tinyxml2::XMLElement* shotgunElement = reader.ForceFindFirstChild(root, "shotgun");

	reader.ForceReadAttribute(reader.ForceFindFirstChild(shotgunElement, "clipsize"), "value", myClipSize);
	reader.ForceReadAttribute(reader.ForceFindFirstChild(shotgunElement, "damage"), "value", myDamage);
	reader.ForceReadAttribute(reader.ForceFindFirstChild(shotgunElement, "startammo"), "value", myAmmoTotal);
	reader.ForceReadAttribute(reader.ForceFindFirstChild(shotgunElement, "minspreadrotation"), "value", myMinSpreadRotation);
	reader.ForceReadAttribute(reader.ForceFindFirstChild(shotgunElement, "maxspreadrotation"), "value", myMaxSpreadRotation);
	myAmmoInClip = myClipSize;

	myShootTime = 2.f;
	myShootTimer = myShootTime;

	reader.CloseDocument();

	myRaycastHandler = [=](Entity* anEntity, const CU::Vector3<float>& aDirection, const CU::Vector3<float>& aHitPosition)
	{
		this->HandleRaycast(anEntity, aDirection, aHitPosition);
	};
}


Shotgun::~Shotgun()
{
}


bool Shotgun::Shoot(const CU::Matrix44<float>& aOrientation)
{
	if (myAmmoInClip > 0 && myShootTimer <= 0.f)
	{
		ShootRowAround(aOrientation, aOrientation.GetForward() * CU::Matrix44<float>::CreateRotateAroundX(CU::Math::RandomRange(-myMaxSpreadRotation, -myMinSpreadRotation)));
		ShootRowAround(aOrientation, aOrientation.GetForward());
		ShootRowAround(aOrientation, aOrientation.GetForward() * CU::Matrix44<float>::CreateRotateAroundX(CU::Math::RandomRange(myMinSpreadRotation, myMaxSpreadRotation)));
		myAmmoInClip -= 1;
		myShootTimer = myShootTime;
		return true;
	}
	return false;
}

void Shotgun::Reload()
{
	int ammoLeft = myAmmoTotal;
	myAmmoTotal -= min(ammoLeft, myClipSize - myAmmoInClip);
	myAmmoInClip = myClipSize;
}

void Shotgun::Update(float aDelta)
{
	myShootTimer -= aDelta;
}

void Shotgun::HandleRaycast(Entity* anEntity, const CU::Vector3<float>& aDirection, const CU::Vector3<float>& aHitPosition)
{
	if (anEntity != nullptr)
	{
		if (anEntity->GetPhysEntity()->GetPhysicsType() == ePhysics::DYNAMIC)
		{
			anEntity->GetPhysEntity()->AddForce(aDirection, 25.f);
		}
		anEntity->SendNote<DamageNote>(DamageNote(myDamage));

		PostMaster::GetInstance()->SendMessage(EmitterMessage("Shotgun", aHitPosition));
	}
}

void Shotgun::ShootRowAround(const CU::Matrix44<float>& aOrientation, const CU::Vector3<float>& aForward)
{
	CU::Vector3<float> forward = aForward;

	Prism::PhysicsInterface::GetInstance()->RayCast(aOrientation.GetPos(), forward, 100.f, myRaycastHandler);

	Prism::PhysicsInterface::GetInstance()->RayCast(aOrientation.GetPos()
		, forward * CU::Matrix44<float>::CreateRotateAroundY(CU::Math::RandomRange(-myMaxSpreadRotation, -myMinSpreadRotation))
		, 100.f, myRaycastHandler);

	Prism::PhysicsInterface::GetInstance()->RayCast(aOrientation.GetPos()
		, forward * CU::Matrix44<float>::CreateRotateAroundY(CU::Math::RandomRange(myMinSpreadRotation, myMaxSpreadRotation))
		, 100.f, myRaycastHandler);
}
