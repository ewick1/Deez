#include "stdafx.h"

#include <Instance.h>
#include "Level.h"
#include <ModelLoader.h>
#include "Player.h"
#include <Scene.h>


Level::Level()
	: myEntities(512)
{
	myPlayer = new Player();
	myScene = new Prism::Scene(*myPlayer->GetCamera());
	myInstance = new Prism::Instance(*Prism::ModelLoader::GetInstance()->LoadModel("Data/Resource/Model/Modular_set/Dev_set/SM_dev_wall_corner_out_200_x_300.fbx", "Data/Resource/Shader/S_effect_pbldebug.fx")
		, myInstanceOrientation);
	myInstanceOrientation = CU::Matrix44<float>::CreateRotateAroundY(M_PI);
	myScene->AddInstance(myInstance);
}


Level::~Level()
{
	myEntities.DeleteAll();
	SAFE_DELETE(myPlayer);
	SAFE_DELETE(myInstance);
	SAFE_DELETE(myScene);
}

void Level::AddEntity(Entity* aEntity)
{
	myEntities.Add(aEntity);
}

void Level::Update(const float aDeltaTime)
{
	myPlayer->Update(aDeltaTime);
}

void Level::Render()
{
	myScene->Render();
}