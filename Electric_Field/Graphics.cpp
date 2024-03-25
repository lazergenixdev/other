#include "Graphics.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma comment( lib,"D3DCompiler.lib" )
#pragma comment( lib, "d3d11.lib" )

void LoadTexture(ID3D11Device* device, const char* filename, ID3D11Texture2D** resource, ID3D11ShaderResourceView** view)
{
	int width, height, channels;
	void* pixels = stbi_load(filename, &width, &height, &channels, 4);

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0u;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MiscFlags = 0u;
	texDesc.Width = UINT(width);
	texDesc.Height = UINT(height);
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1u;
	texDesc.SampleDesc.Quality = 0u;
	texDesc.ArraySize = 1u;
	D3D11_SUBRESOURCE_DATA srd;
	srd.pSysMem = pixels;
	srd.SysMemPitch = UINT(width * 4);
	srd.SysMemSlicePitch = UINT(width * height * 4);
	device->CreateTexture2D( &texDesc, &srd, resource );
	device->CreateShaderResourceView(*resource, nullptr, view);
}

Graphics::Graphics(const Window & wnd)
	: pWindow( &wnd )
{
	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = 1;
	scd.BufferDesc.RefreshRate.Denominator = 60;
	scd.BufferDesc.Width = pWindow->ScreenWidth;
	scd.BufferDesc.Height = pWindow->ScreenHeight;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = pWindow->hWnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed = TRUE;

	int flag = 0;
#ifdef _DEBUG
	flag |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D11CreateDeviceAndSwapChain(nullptr , D3D_DRIVER_TYPE_HARDWARE, nullptr, flag, nullptr, 0,
		D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, nullptr, &pImmediateContext );

	/******* Setup Quad for rendering *******/
	struct VertexPTC
	{
		float x, y, u, v;
	};
	VertexPTC quad [] =
	{
		{ -1.0f, 1.0f, 0.0f, 0.0f },
		{  1.0f, 1.0f, 1.0f, 0.0f },
		{  1.0f,-1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, 0.0f, 0.0f },
		{  1.0f,-1.0f, 1.0f, 1.0f },
		{ -1.0f,-1.0f, 0.0f, 1.0f },
	};
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = sizeof(VertexPTC) * (UINT)std::size( quad );
	bd.CPUAccessFlags = 0u;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.StructureByteStride = sizeof(VertexPTC);
	bd.MiscFlags = 0u;
	D3D11_SUBRESOURCE_DATA srd = {};
	srd.pSysMem = quad;
	pDevice->CreateBuffer( &bd, &srd, &pVertexBuffer );

	/******* Create Vertex Shader with shader layout *******/
	Microsoft::WRL::ComPtr<ID3DBlob> pBlob;
	D3DReadFileToBlob( L"VS.cso", &pBlob );
	pDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader );
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		{ "TexCoord", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 8u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
	};
	pDevice->CreateInputLayout( ied, 2u, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &pInputLayout);
	
	/******* Set Viewport *******/
	D3D11_VIEWPORT vp;
	vp.Width = float( pWindow->ScreenWidth );
	vp.Height = float( pWindow->ScreenHeight );
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	pImmediateContext->RSSetViewports( 1, &vp );

	/******* Create Pixel Shader *******/
	D3DReadFileToBlob(L"PS.cso", &pBlob);
	pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader);

	/******* Create Texture for PS to render quad *******/
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0u;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MiscFlags = 0u;
	texDesc.Width = pWindow->ScreenWidth;
	texDesc.Height = pWindow->ScreenHeight;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1u;
	texDesc.SampleDesc.Quality = 0u;
	texDesc.ArraySize = 1u;
	pDevice->CreateTexture2D( &texDesc, nullptr, &pTexture );
	pDevice->CreateShaderResourceView(pTexture.Get(), nullptr, &pResourceView);

	D3D11_SAMPLER_DESC smplrDesc = {};
	smplrDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	smplrDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	smplrDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	smplrDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	smplrDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	smplrDesc.MinLOD = 0.0f;
	smplrDesc.MaxLOD = D3D11_FLOAT32_MAX;
	pDevice->CreateSamplerState( &smplrDesc, &pSampler );

	/******* Get Back Buffer and Set as RenderTarget *******/
	Microsoft::WRL::ComPtr<ID3D11Resource> pBackBuffer;
	pSwapChain->GetBuffer(0,
		__uuidof(ID3D11Texture2D),
		(LPVOID*)& pBackBuffer);
	pDevice->CreateRenderTargetView( pBackBuffer.Get(), nullptr, &pRenderTarget );
	pImmediateContext->OMSetRenderTargets(1u, pRenderTarget.GetAddressOf(), nullptr);

	/******* Create Compute Shader *******/
	D3DReadFileToBlob(L"ComputeShader.cso", &pBlob);
	pDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pComputeShader);

	/******* Create Texture for CS to write to with UAV *******/
	D3D11_TEXTURE2D_DESC fieldTexDesc = {};
	fieldTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	fieldTexDesc.CPUAccessFlags = 0u;
	fieldTexDesc.MiscFlags = 0;
	fieldTexDesc.Usage = D3D11_USAGE_DEFAULT;
	fieldTexDesc.ArraySize = 1u;
	fieldTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	fieldTexDesc.SampleDesc.Count = 1u;
	fieldTexDesc.SampleDesc.Quality = 0u;
	fieldTexDesc.Width = pWindow->ScreenWidth;
	fieldTexDesc.Height = pWindow->ScreenHeight;
	fieldTexDesc.MipLevels = 1u;
	pDevice->CreateTexture2D( &fieldTexDesc, nullptr, &pFieldTexture);
	pDevice->CreateUnorderedAccessView(pFieldTexture.Get(), nullptr, &pFieldTextureView);

	buffer.select.r = 0;
	buffer.select.g = 1;
	buffer.select.b = 0;
	buffer.select.w = 1;
	buffer.particles[0] = { 200.0f, 350.0f, 1.0f };
	buffer.particles[1] = { 800.0f, 350.0f, -1.0f };
	buffer.particles[2] = { 400.0f, 350.0f, 1.0f };
	buffer.particles[3] = { 600.0f, 150.0f, -1.0f };
	buffer.particles[4] = { 600.0f, 550.0f, -1.0f };

	/******* Create Constant Buffer of particle info *******/
	D3D11_BUFFER_DESC cbufDesc = {};
	cbufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufDesc.ByteWidth = sizeof( CBuf );
	cbufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufDesc.StructureByteStride = 0u;
	cbufDesc.Usage = D3D11_USAGE_DYNAMIC;
	srd.pSysMem = &buffer;
	srd.SysMemPitch = 0;
	pDevice->CreateBuffer( &cbufDesc, &srd, &pConstantBuffer);

	LoadTexture( pDevice.Get(), "electron.png", &pElectron, &pElectronView );
	LoadTexture( pDevice.Get(), "proton.png", &pProton, &pProtonView );
}

void Graphics::BeginFrame(const Color& initialColor)
{
	float color [] = { 0,0,0,1 };
	pImmediateContext->ClearUnorderedAccessViewFloat( pFieldTextureView.Get(), color );
}

void Graphics::EndFrame()
{
	// setup compute shader
	pImmediateContext->CSSetShader( pComputeShader.Get(), nullptr, 0u );
	pImmediateContext->CSSetUnorderedAccessViews( 0u, 1u, pFieldTextureView.GetAddressOf(), 0u );
	pImmediateContext->CSSetConstantBuffers( 0u, 1u, pConstantBuffer.GetAddressOf() );
	// run compute shader to compute field lines to texture
	pImmediateContext->Dispatch( 1, 1, 1 );
	// copy texture to resource that the pixel shader can use
	pImmediateContext->CopyResource( pTexture.Get(), pFieldTexture.Get() );
	// setup gfx pipeline
	UINT stride = 16u;
	UINT offset = 0u;
	pImmediateContext->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);
	pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pImmediateContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);
	pImmediateContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);
	pImmediateContext->IASetInputLayout(pInputLayout.Get());

	ID3D11ShaderResourceView* ppResources[] =
	{
		pResourceView.Get(),
		pProtonView.Get(),
		pElectronView.Get(),
	};

	pImmediateContext->PSSetShaderResources( 0u, 3u, ppResources );
	pImmediateContext->PSSetSamplers(0u, 1u, pSampler.GetAddressOf());
	pImmediateContext->PSSetConstantBuffers( 0u, 1u, pConstantBuffer.GetAddressOf() );
	// render quad
	pImmediateContext->Draw( 6u, 0u );
	// present back buffer to the output window
	pSwapChain->Present( 1u, 0u );
}

void Graphics::TestMoveSelected(float delta_x, float delta_y)
{
	buffer.particles[buffer.select.w].x += delta_x;
	buffer.particles[buffer.select.w].y += delta_y;

	D3D11_MAPPED_SUBRESOURCE msr;
	pImmediateContext->Map(pConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr);

	memcpy( msr.pData, &buffer, sizeof(CBuf) );

	pImmediateContext->Unmap( pConstantBuffer.Get(), 0u );
}

void Graphics::TestSwitchSelected()
{
	buffer.select.w++;
	buffer.select.w %= 5;
}
