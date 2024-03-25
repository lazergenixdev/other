#pragma once
#include "Window.h"
#include <d3dcompiler.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include "Color.h"

class Graphics
{
public:
	Graphics( const Window& wnd );
	void BeginFrame( const Color& initialColor );
	void EndFrame();
	void TestMoveSelected( float delta_x, float delta_y );
	void TestSwitchSelected();
private:
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain;
	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pImmediateContext;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTarget;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pResourceView;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> pInputLayout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> pSampler;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> pComputeShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> pConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pFieldTexture;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> pFieldTextureView;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> pElectron;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pProton;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pElectronView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pProtonView;
	struct CBuf
	{
		//int nLines;
		struct int4
		{
			int r, g, b, w; // w -> index of selected | r, g, b -> color
		} select;
		struct float4
		{
			float x, y, z, w = 1.0f;
		} particles[16];
	} buffer;
	const Window* pWindow;
};