#include "stdafx.h"
#include "Camera.h"
#include <CommonHelper.h>
#include "Frustum.h"
#include <InputWrapper.h>
#include "Instance.h"
#include <Intersection.h>
#include "Portal.h"
#include "Room.h"
#include "RoomManager.h"

namespace Prism
{
	RoomManager::RoomManager()
		: myRooms(128)
		, myPortals(128)
		, myCurrentRoomIds(16)
		, myInstances(4096)
		, myAlwaysRenderInstances(128)
		, myActiveInstances(4096)
		, myDebugDraw(false)
	{
	}


	RoomManager::~RoomManager()
	{
		myActiveInstances.RemoveAll();
		myInstances.RemoveAll();
	}

	void RoomManager::Add(Room* aRoom)
	{
		myRooms.Add(aRoom);
	}

	static bool cmp(Room* a, Room* b)
	{
		return int(a->GetType())< int(b->GetType());
	}


	void RoomManager::CalcPortals()
	{
		std::sort(myRooms.begin(), myRooms.end(), cmp);

		for (int i = 0; i < myRooms.Size(); ++i)
		{
			myRooms[i]->SetRoomId(i);
		}

		for (int i = 0; i < myRooms.Size(); ++i)
		{
			for (int j = i + 1; j < myRooms.Size(); ++j)
			{
				if (myRooms[i]->Collide(*myRooms[j]) == true)
				{
					myPortals.Add(new Portal(myRooms[i], myRooms[j]));
				}
			}
		}

		//bool success = false;
		//while (success == false)
		//{
		//	success = true;
		//	for (int i = 0; i < myRooms.Size() - 1; ++i)
		//	{
		//		if (myRooms[i + 1] < myRooms[i])
		//		{
		//			std::swap(myRooms[i], myRooms[i + 1]);
		//			success = false;
		//		}
		//	}
		//}
	}

	void RoomManager::Add(Instance* anInstance, bool anAlwaysRender)
	{
		if (anAlwaysRender == true)
		{
			myAlwaysRenderInstances.Add(anInstance);
		}
		else
		{
			bool success(false);
			for (int i = 0; i < myRooms.Size(); ++i)
			{
				if (myRooms[i]->Inside(anInstance->GetPosition()) == true)
				{
					myInstances.Add(InstanceInRoom(i, anInstance));
					success = true;
					break;
				}
			}

			DL_ASSERT_EXP(success == true, CU::Concatenate("Instance found outside room: (x: %f y: %f z: %f)"
				, anInstance->GetOrientation().GetPos().x
				, anInstance->GetOrientation().GetPos().y
				, anInstance->GetOrientation().GetPos().z));
		}
	}

	void RoomManager::Remove(Instance* anInstance)
	{
		for (int i = 0; i < myInstances.Size(); ++i)
		{
			if (myInstances[i].myInstance == anInstance)
			{
				myInstances.RemoveCyclicAtIndex(i);
				return;
			}
		}

		//DL_ASSERT("Unable to remove instance that does not exist in Room Manager.");
	}

	const CU::GrowingArray<Instance*>& RoomManager::GetActiveInstances(const Camera& aCamera)
	{
#ifndef RELEASE_BUILD
		if (CU::InputWrapper::GetInstance()->KeyDown(DIK_P) == true)
		{
			myDebugDraw = !myDebugDraw;
		}
#endif
		myActiveInstances.RemoveAll();
		myCurrentRoomIds.RemoveAll();

		for (int i = 0; i < myPortals.Size(); ++i)
		{
			myPortals[i]->SetAlreadyPassed(false);
		}

		int playerRoom = GetRoomId(aCamera.GetOrientation().GetPos());
		myCurrentRoomIds.Add(playerRoom);

		FindActiveRooms(aCamera.GetFrustum(), playerRoom);

		//for (int i = 0; i < myCurrentRoomIds.Size(); ++i)
		//{
		//	const Room* current(myRooms[myCurrentRoomIds[i]]);
		//	if (current->GetType() == eRoomType::ROOM)
		//	{
		//		for (int j = 0; j < current->GetPortals().Size(); ++j)
		//		{
		//			const Room* other(current->GetPortals()[j]->GetOther(current));
		//			if (myCurrentRoomIds.Find(other->GetRoomId()) == myCurrentRoomIds.FoundNone)
		//			{
		//				myCurrentRoomIds.Add(other->GetRoomId());
		//			}
		//		}
		//	}
		//}

		for (int i = 0; i < myAlwaysRenderInstances.Size(); ++i)
		{
			myActiveInstances.Add(myAlwaysRenderInstances[i]);
		}
		for (int i = 0; i < myInstances.Size(); ++i)
		{
			for (int j = 0; j < myCurrentRoomIds.Size(); ++j)
			{
				if (myCurrentRoomIds[j] == myInstances[i].myRoomId)
				{
					myActiveInstances.Add(myInstances[i].myInstance);
					break;
				}
			}
		}

		

#ifndef RELEASE_BUILD
#ifdef SHOW_PORTAL_CULLING_DEBUG_TEXT
		//DEBUG_PRINT(playerRoom);
		DEBUG_PRINT(myActiveInstances.Size());
		DEBUG_PRINT(myInstances.Size());
		float renderPercentage = 100.f * float(myActiveInstances.Size())
			/ (myInstances.Size() + myAlwaysRenderInstances.Size());
		DEBUG_PRINT(renderPercentage);
		for (int j = 0; j < myCurrentRoomIds.Size(); ++j)
		{
			const std::string& roomName(myRooms[myCurrentRoomIds[j]]->GetName());
			if(j == 0)
			{
				const std::string& playerRoom(roomName);
				DEBUG_PRINT(playerRoom);
			}
			DEBUG_PRINT(roomName);
		}
		if (renderPercentage > 25.f)
		{
			for (float i = 25.f; i < renderPercentage; i += 5.f)
			{
				DEBUG_PRINT("WARNING, rendering huge part of level");

			}
		}
#endif
#endif
		return myActiveInstances;
	}

	int RoomManager::GetRoomId(const CU::Vector3<float>& aPosition) const
	{
		for (int i = 0; i < myRooms.Size(); ++i)
		{
			if (myRooms[i]->Inside(aPosition) == true)
			{
				return i;
			}
		}

		DL_ASSERT("Unable to find room Id");
		return 0;
	}

	void RoomManager::FindActiveRooms(Frustum aFrustum, int aRoomId, Portal* anArrivePortal)
	{
		if (anArrivePortal != nullptr)
		{
			aFrustum.Resize(anArrivePortal, myDebugDraw);

		}



		if (aFrustum.GetBottomLeft() != CU::Vector3<float>(0, 0, 0))
		{
			//Prism::DebugDrawer::GetInstance()->RenderLine3D(aFrustum.GetOrientation().GetPos(), aFrustum.GetBottomLeft(), eColorDebug::RED);
			//Prism::DebugDrawer::GetInstance()->RenderLine3D(aFrustum.GetOrientation().GetPos(), aFrustum.GetTopRight(), eColorDebug::BLUE);
		}

		const CU::GrowingArray<Portal*>& portals = myRooms[aRoomId]->GetPortals();

		for (int i = 0; i < portals.Size(); ++i)
		{
			Portal* current = portals[i];
			int planeOutside[4];
			bool pointBehind[4];




			if (current->GetAlreadyPassed() == false
				&& (aFrustum.Inside(current->GetPoint(0), 0, planeOutside[0], pointBehind[0]) == true
				|| aFrustum.Inside(current->GetPoint(1), 0, planeOutside[1], pointBehind[1]) == true
				|| aFrustum.Inside(current->GetPoint(2), 0, planeOutside[2], pointBehind[2]) == true
				|| aFrustum.Inside(current->GetPoint(3), 0, planeOutside[3], pointBehind[3]) == true

				//|| AllPointsBehind(pointBehind) == false // fancy solution, still not 100% working
				//&& (AnyOutsidePlane(planeOutside, 0) == true && AnyOutsidePlane(planeOutside, 1)
				//|| AnyOutsidePlane(planeOutside, 2) == true && AnyOutsidePlane(planeOutside, 3))))
				
				|| AllPointsBehind(pointBehind) == false && !(planeOutside[0] == planeOutside[1] && planeOutside[0] == planeOutside[2] && planeOutside[0] == planeOutside[3]))) //naive solution
				
				//|| aFrustum.Inside(current->GetCenterPosition(), current->GetRadius() * 0.1f) == true)) // needs to be tweaked for portal size
			{
				if (myDebugDraw == true)
				{
					Prism::DebugDrawer::GetInstance()->RenderLine3D(current->GetPoint(0), current->GetPoint(1), eColorDebug::RED, eColorDebug::BLUE);
					Prism::DebugDrawer::GetInstance()->RenderLine3D(current->GetPoint(1), current->GetPoint(3), eColorDebug::BLUE, eColorDebug::GREEN);
					Prism::DebugDrawer::GetInstance()->RenderLine3D(current->GetPoint(3), current->GetPoint(2), eColorDebug::GREEN, eColorDebug::YELLOW);
					Prism::DebugDrawer::GetInstance()->RenderLine3D(current->GetPoint(2), current->GetPoint(0), eColorDebug::YELLOW, eColorDebug::RED);
				}
				current->SetAlreadyPassed(true);

				int otherRoomId = current->GetOther(myRooms[aRoomId])->GetRoomId();
				if (myCurrentRoomIds.Find(otherRoomId) == myCurrentRoomIds.FoundNone)
				{
					myCurrentRoomIds.Add(otherRoomId);

					FindActiveRooms(aFrustum, otherRoomId, current);
				}
			}
		}
	}

	bool RoomManager::AnyOutsidePlane(int somePlaneIndices[4], int aPlaneIndex) const
	{
		return somePlaneIndices[0] == aPlaneIndex
			|| somePlaneIndices[1] == aPlaneIndex
			|| somePlaneIndices[2] == aPlaneIndex
			|| somePlaneIndices[3] == aPlaneIndex;
	}

	bool RoomManager::AllPointsBehind(bool somePoints[4]) const
	{
		return somePoints[0] == true
			&& somePoints[1] == true
			&& somePoints[2] == true
			&& somePoints[3] == true;
	}
}