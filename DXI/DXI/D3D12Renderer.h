//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_RENDERER_H_INCLUDED
#define D3D12_RENDERER_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"
#include "Material.h"
#include "GBuffer.h"
#include "ShadowMap.h"
#include "CubeMap.h"

// Core | Everything every D3D12 app would otherwise duplicate
#include "Core/D3D12Device.h"
#include "Core/D3D12SwapChain.h"
#include "Core/D3D12DepthBuffer.h"
#include "Core/D3D12FrameContext.h"

// Objects
#include "Objects/Camera.h"
#include "Objects/Plane.h"
#include "Objects/Cube.h"

// Lights
#include "LightTypes/DirectionalLight.h"
#include "LightTypes/PointLight.h"
#include "LightTypes/AreaLight.h"

// Passes
#include "Passes/ShadowPass.h"
#include "Passes/BasePass.h"
#include "Passes/LightPass.h"


// Deferred renderer.
// Same surface as D3D12App so Win32Proc can drive either of them, but the D3D12
// plumbing lives in the Core classes and the frame is a BasePass into the G-buffer
// followed by a LightPass that shades it.
class D3D12Renderer
{
	DISABLE_COPY_MOVE(D3D12Renderer)

public:

	D3D12Renderer() = delete;
	// Ctors & DCtors
	D3D12Renderer(UINT WindowWidth, UINT WindowHeight, std::wstring WindowName);
	~D3D12Renderer() = default; //just do nothing, the members take care of deallocation

	// Getters
	_NODISCARD UINT					GetWindowWidth() const	{ return WindowWidth; }
	_NODISCARD UINT					GetWindowHeight() const	{ return WindowHeight; }
	_NODISCARD float				GetAspectRatio() const	{ return AspectRatio; }
	_NODISCARD const std::wstring&	GetWindowName() const	{ return WindowName; }

	// Functions
	void Initialize();
	void Render();
	void Update(float DeltaTime);
	void Destroy();

	// Movement of camera
	void OnKeyDown(UINT8 Key);
	void OnKeyUp(UINT8 Key);
	void OnMouseMove(int DeltaX, int DeltaY);

	// Presentation | With vsync on the frame rate can never exceed the refresh rate of
	// the display, which is 32 Hz over a remote desktop session
	void SetVSyncEnabled(const bool bEnabled) noexcept
	{
		bVSyncEnabled = bEnabled;
	}
	_NODISCARD bool IsVSyncEnabled() const noexcept
	{
		return bVSyncEnabled;
	}

private:

	// Initialization, split per concern instead of one long function
	void InitializeConstantBuffers();
	void InitializeMaterials();
	void InitializeScene();
	void InitializePasses();

	// Loads one material out of a Poly Haven style folder, where every map is named
	// <AssetName>_<suffix>_1k.png, appends it to Materials and hands it back.
	// Missing maps keep their null descriptor.
	Material* LoadMaterial(const wchar_t* AssetName);

	// Frame
	void BeginFrame();
	void EndFrame();

	// Passes
	void RenderShadowPass();
	void RenderBasePass();
	void RenderLightPass();

	// Light of the scene the shadow map is rendered from, null when nothing casts
	_NODISCARD const DirectionalLight* GetShadowCaster(INT& OutLightIdx) const;

	// Uploads
	void UploadCameraConstants();
	void UploadLightConstants();
	void UploadPrimitiveConstants(const Primitive& Primitive, const UINT PrimitiveIdx);

	// GPU address of a primitive's constants for the frame being recorded
	_NODISCARD D3D12_GPU_VIRTUAL_ADDRESS GetPrimitiveCbvAddress(const SIZE_T PrimitiveIdx) const;

	// Constants / Misc
	static constexpr UINT	BACK_BUFFER_COUNT	= 3;											//how many frames are in flight
	static constexpr UINT	MAX_MATERIALS		= 10;											//how many materials fit in the shared descriptor heap
	static constexpr UINT	CAMERA_CBV_STRIDE	= CameraConstantBufferSize;							//already a multiple of the constant buffer alignment
	static constexpr UINT	SKY_FACE_RESOLUTION	= 512;												//per radiance cube face, projected out of the panorama on the CPU
	static constexpr UINT	SKY_IRRADIANCE_RESOLUTION = 32;											//irradiance barely varies with direction, so it stays tiny
	static constexpr UINT	SHADOW_MAP_RESOLUTION = 2048;										//square shadow map covering the whole scene

	// Window Properties
	UINT					WindowWidth;
	UINT					WindowHeight;
	float					AspectRatio;
	std::wstring			WindowName;

	// D3D12 Window Properties
	CD3DX12_VIEWPORT		ViewPort;
	CD3DX12_RECT			ScissorRect;

	// Core | Device, presentation, depth and command recording
	D3D12Device				Device;
	D3D12SwapChain			SwapChain;
	D3D12DepthBuffer		DepthBuffer;
	D3D12FrameContext		FrameContext;
	UINT					CurrentFrameIdx = 0;

	// Off by default, otherwise the frame rate is pinned to the refresh rate and says
	// nothing about how long the frame actually takes
	bool					bVSyncEnabled	= false;

	// Deferred rendering | ShadowPass writes depth from the light, BasePass writes the
	// surface, LightPass shades it and looks the occlusion up in the shadow map
	GBuffer					SceneGBuffer;
	ShadowMap				SceneShadowMap;

	// Sky of the scene. The radiance cube is both the background and what the specular
	// reflection reads, the irradiance cube is what the diffuse reflection reads.
	CubeMap					SkyRadianceMap;
	CubeMap					SkyIrradianceMap;
	ShadowPass				SceneShadowPass;
	BasePass				SceneBasePass;
	LightPass				SceneLightPass;

	// Everything LightPass samples has to live in one shader visible heap, because only
	// one CBV/SRV/UAV heap can be bound at a time
	ComPtr<DXDescriptorHeap>	LightInputsHeap;

	// Materials | Every material takes its descriptors out of the one shared heap,
	// one per primitive so each object looks different
	MaterialHeap							SharedMaterialHeap;
	std::vector<std::unique_ptr<Material>>	Materials;

	// Scene
	Camera									Camera;
	std::vector<std::unique_ptr<Primitive>>	Primitives;
	std::vector<std::unique_ptr<LightBase>>	Lights;

	// Constant buffers | One region per frame in flight, all of them permanently mapped
	ComPtr<DXResource>		PrimitiveCBV;					// one PrimitiveConstantBuffer per primitive, per frame
	UINT8*					PrimitiveCbvAddress	= nullptr;
	ComPtr<DXResource>		CameraCBV;						// eye position, per frame
	UINT8*					CameraCbvAddress	= nullptr;
	ComPtr<DXResource>		LightsCBV;						// every light of the scene, per frame
	UINT8*					LightsCbvAddress	= nullptr;

	// Timer | Win32Proc hands Update() a zero delta, so the timer is what actually drives movement
	Helpers::StepTimer		StepTimer;
};

#endif // D3D12_RENDERER_H_INCLUDED
