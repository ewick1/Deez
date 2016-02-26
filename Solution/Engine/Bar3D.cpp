#include "stdafx.h"
#include "Bar3D.h"
#include "Camera.h"
#include "IndexBufferWrapper.h"
#include <d3dx11effect.h>
#include "Surface.h"

namespace Prism
{
	Bar3D::Bar3D(const CU::Vector2<float>& aPositionFromJoint, const CU::Vector2<float>& aQuadSize
		, int aNumberOfQuads, Effect* aEffect, eBarPosition aBarPosition)
		: myQuadSize(aQuadSize)
		, myValue(0.f)
		, myNbrOfQuads(aNumberOfQuads)
		, myBarPosition(aBarPosition)
	{
		myEffect = aEffect;
		D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};


		InitInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), "GUI::InputLayout");
		InitVertexBuffer(sizeof(GUIVertex), D3D11_USAGE_IMMUTABLE, 0);

		//InitIndexBuffer();
		InitSurface("AlbedoTexture", "Data/Resource/Texture/T_missing_texture.dds");
		mySurfaces[0]->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		InitBlendState("GUI::BlendState");

		CreateVertices(aPositionFromJoint, aNumberOfQuads);
	}


	Bar3D::~Bar3D()
	{
	}

	void Bar3D::Render(const Camera& aCamera, const CU::Matrix44<float>& aWorld)
	{
		Prism::Engine::GetInstance()->DisableCulling();
		myEffect->SetWorldMatrix(aWorld);
		myEffect->SetViewProjectionMatrix(aCamera.GetViewProjection());
		myEffect->SetCameraPosition(aCamera.GetOrientation().GetPos());
		//BaseModel::Render();
		Render();
		Prism::Engine::GetInstance()->EnableCulling();
	}

	void Bar3D::Render()
	{

		Engine::GetInstance()->GetContex()->IASetInputLayout(myVertexLayout);
		Engine::GetInstance()->GetContex()->IASetVertexBuffers(myVertexBuffer->myStartSlot
			, myVertexBuffer->myNumberOfBuffers, &myVertexBuffer->myVertexBuffer
			, &myVertexBuffer->myStride, &myVertexBuffer->myByteOffset);
		//Engine::GetInstance()->GetContex()->IASetIndexBuffer(myIndexBuffer->myIndexBuffer
		//	, myIndexBuffer->myIndexBufferFormat, myIndexBuffer->myByteOffset);

		for (int s = 0; s < mySurfaces.Size(); ++s)
		{
			int verticesToSend = abs(myValue) * myNbrOfQuads;
			int rest = verticesToSend * 2;
			verticesToSend = rest + 2;
			mySurfaces[s]->Activate();

			ID3DX11EffectTechnique* tech = nullptr;
			tech = myEffect->GetTechnique(myTechniqueName);

			if (tech->IsValid() == false)
			{
				tech = myEffect->GetTechnique();
				DL_ASSERT("INVALID TECHNIQUE IN BASEMODEL::RENDER: " + myTechniqueName);
			}

			DL_ASSERT_EXP(tech != nullptr, "Technique is nullptr");
			DL_ASSERT_EXP(tech->IsValid() != false, "Technique is not valid");

			D3DX11_TECHNIQUE_DESC techDesc;
			tech->GetDesc(&techDesc);
			for (UINT i = 0; i < techDesc.Passes; ++i)
			{
				tech->GetPassByIndex(i)->Apply(0, Engine::GetInstance()->GetContex());
				/*Engine::GetInstance()->GetContex()->DrawIndexed(indicesToSend
					, mySurfaces[s]->GetIndexStart(), 0);*/
				Engine::GetInstance()->GetContex()->Draw(verticesToSend, 0);
			}

			mySurfaces[s]->DeActivate();
		}
	}

	void Bar3D::SetValue(float aValue)
	{
		myValue = aValue;
	}

	void Bar3D::CreateVertices(const CU::Vector2<float>& aPositionFromJoint, int aNumberOfQuads)
	{
		CU::GrowingArray<GUIVertex> vertices(4 * aNumberOfQuads);
		CU::GrowingArray<CU::Vector4<float>> torusPosition(4 * aNumberOfQuads);

		float inner = 0.05f;
		float outer = 0.1f;
		if (myBarPosition == eBarPosition::LEFT || myBarPosition == eBarPosition::RIGHT || myBarPosition == eBarPosition::TOP)
		{
			for (int i = 0; i < aNumberOfQuads; ++i)
			{
				float angle = 0.f;
				if (myBarPosition == eBarPosition::LEFT)
				{
					angle = (i / (float)aNumberOfQuads) * 2.093f;
					angle += 1.64f;// *1.5f;
					angle = -angle;
				}
				else if (myBarPosition == eBarPosition::RIGHT)
				{
					angle = (i / (float)aNumberOfQuads) * 2.093f;
					angle -= 1.582f;// *1.5f;
				}
				else if (myBarPosition == eBarPosition::TOP)
				{
					angle = (i / (float)aNumberOfQuads) * 2.093f;
					angle += 3.72f;// *1.5f;
					angle = -angle;
				}
				CU::Vector4<float> vertexInner(inner * cos(angle), inner * sin(angle), 0.f, 0.f);
				CU::Vector4<float> vertexOuter(outer * cos(angle), outer * sin(angle), 0.f, 0.f);

				torusPosition.Add(vertexInner);
				torusPosition.Add(vertexOuter);
			}

			for (int i = 0; i < torusPosition.Size(); ++i)
			{
				GUIVertex vertex;

				if (i == 0)
				{
					vertex.myPosition = torusPosition[0];
					vertex.myUV = { 1, 0 };
				}
				else if (i == 1)
				{
					vertex.myPosition = torusPosition[1];
					vertex.myUV = { 0, 0 };
				}
				else if (i == 2)
				{
					vertex.myPosition = torusPosition[2];
					vertex.myUV = { 1, 1 };
				}
				else if (i == 3)
				{
					vertex.myPosition = torusPosition[3];
					vertex.myUV = { 0, 1 };
				}
				else
				{
					if (i % 2 == 0)
					{
						vertex.myPosition = torusPosition[i];
						vertex.myUV = { 1, 1 };
					}
					else
					{
						vertex.myPosition = torusPosition[i];
						vertex.myUV = { 0, 1 };
					}
				}

				vertices.Add(vertex);
			}
		}
		else
		{
			for (int i = 0; i < aNumberOfQuads; ++i)
			{
				GUIVertex vertex;
				if (i == 0)
				{
					vertex.myPosition = { aPositionFromJoint.x + myQuadSize.x, aPositionFromJoint.y, 0, 0 };
					vertex.myPosition.y += myQuadSize.y * i;
					vertex.myUV = { 1, 0 };
					vertices.Add(vertex);

					vertex.myPosition = { aPositionFromJoint.x, aPositionFromJoint.y, 0, 0 };
					vertex.myPosition.y += myQuadSize.y * i;
					vertex.myUV = { 0, 0 };
					vertices.Add(vertex);

					vertex.myPosition = { aPositionFromJoint.x + myQuadSize.x, aPositionFromJoint.y + myQuadSize.y, 0, 0 };
					vertex.myPosition.y += myQuadSize.y * i;
					vertex.myUV = { 1, 1 };
					vertices.Add(vertex);

					vertex.myPosition = { aPositionFromJoint.x, aPositionFromJoint.y + myQuadSize.y, 0, 0 };
					vertex.myPosition.y += myQuadSize.y * i;
					vertex.myUV = { 0, 1 };
					vertices.Add(vertex);
				}
				else
				{
					vertex.myPosition = { aPositionFromJoint.x + myQuadSize.x, aPositionFromJoint.y, 0, 0 };
					vertex.myPosition.y += myQuadSize.y * i;
					vertex.myUV = { 1, 0 };
					vertices.Add(vertex);

					vertex.myPosition = { aPositionFromJoint.x, aPositionFromJoint.y, 0, 0 };
					vertex.myPosition.y += myQuadSize.y * i;
					vertex.myUV = { 0, 0 };
					vertices.Add(vertex);
				}

			}
		}

		SetupVertexBuffer(vertices.Size(), sizeof(GUIVertex), reinterpret_cast<char*>(&vertices[0])
			, "GUI::VertexBuffer");

		mySurfaces[0]->SetVertexCount(vertices.Size());
	}
}