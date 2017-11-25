/*
Redo the Colored Cube demo, but this time use two vertex buffers (and two input slots) to feed the pipeline with vertices, 
one that stores the position element and the other that stores the color element. 
For this you will use two vertex structures to store the split data: 

struct VPosData
{
  XMFLOAT3 Pos;
};
 
struct VColorData
{
  XMFLOAT4 Color;
};

Your D3D12_INPUT_ELEMENT_DESC array will look like this:

D3D12_INPUT_ELEMENT_DESC vertexDesc[] =
{
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, 
    D3D12_INPUT_PER_VERTEX_DATA, 0},
  {"COLOR",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, 
    D3D12_INPUT_PER_VERTEX_DATA, 0}
};

The position element is hooked up to input slot 0, and the color element is hooked up to input slot 1. 
Moreover note that the D3D12_INPUT_ELEMENT_DESC::AlignedByteOffset is 0 for both elements; 
this is because the position and color elements are no longer interleaved in a single input slot. 
Then use ID3D12CommandList::IASetVertexBuffers to bind the two vertex buffers to slots 0 and 1. 
Direct3D will then use the elements from the different input slots to assemble the vertices. 
*/


//***************************************************************************************
// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
//   Hold the left mouse button down and move the mouse to rotate.
//   Hold the right mouse button down and move the mouse to zoom in and out.
//***************************************************************************************

....

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
    BoxApp(const BoxApp& rhs) = delete;
    BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void BuildDescriptorHeaps();
	void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

private:
    
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    POINT mLastMousePos;

	//*************Chapter 6 exercise 2*****************************************************
	std::vector<D3D12_VERTEX_BUFFER_VIEW> bufferview;
	ComPtr<ID3D12Resource> VerPosBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VerPosBufferUploader = nullptr;

	ComPtr<ID3D12Resource> VerColorBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VerColorBufferUploader = nullptr;
	//*******************Change*************************************************************
};

void BoxApp::Draw(const GameTimer& gt)
{
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
    // Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	//*************Chapter 6 exercise 2*****************************************************
	mCommandList->IASetVertexBuffers(0, 1, &bufferview.at(0));
	mCommandList->IASetVertexBuffers(1, 1, &bufferview.at(1));
	//*******************Change*************************************************************

	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    mCommandList->DrawIndexedInstanced(
		mBoxGeo->DrawArgs["box"].IndexCount, 
		1, 0, 0, 0);
	
    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
	ThrowIfFailed(mCommandList->Close());
 
    // Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	
	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

void BoxApp::BuildBoxGeometry()
{
	//*************Chapter 6 exercise 2*****************************************************
    std::array<VPosData, 8> vertices =
    {
		VPosData({ XMFLOAT3(-1.0f, -1.0f, -1.0f),  }),
		VPosData({ XMFLOAT3(-1.0f, +1.0f, -1.0f), }),
		VPosData({ XMFLOAT3(+1.0f, +1.0f, -1.0f),  }),
		VPosData({ XMFLOAT3(+1.0f, -1.0f, -1.0f),  }),
		VPosData({ XMFLOAT3(-1.0f, -1.0f, +1.0f),  }),
		VPosData({ XMFLOAT3(-1.0f, +1.0f, +1.0f),  }),
		VPosData({ XMFLOAT3(+1.0f, +1.0f, +1.0f),  }),
		VPosData({ XMFLOAT3(+1.0f, -1.0f, +1.0f),  })
    };
	std::array<VColorData, 8> colors = {
		VColorData{XMFLOAT4(Colors::Snow) },
		VColorData{ XMFLOAT4(Colors::Black) },
		VColorData{ XMFLOAT4(Colors::Pink) },
		VColorData{ XMFLOAT4(Colors::BlueViolet) },
		VColorData{ XMFLOAT4(Colors::Red) },
		VColorData{ XMFLOAT4(Colors::LightGoldenrodYellow) },
		VColorData{ XMFLOAT4(Colors::DeepSkyBlue) },
		VColorData{ XMFLOAT4(Colors::ForestGreen) },
	};
	//*************Chapter 6 exercise 2*****************************************************
	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(VPosData);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	//*************Chapter 6 exercise 2*****************************************************
	const UINT vColorByteSize = (UINT)colors.size() * sizeof(VColorData);
	//*************Chapter 6 exercise 2*****************************************************

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";


	//ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	//CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	//***********************Chapter 6 exercise 2*************************************************************************
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	VerPosBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, VerPosBufferUploader);

	VerColorBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), colors.data(), vColorByteSize, VerColorBufferUploader);

	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	UINT VertexByteStride = sizeof(VPosData);
	UINT VertexBufferByteSize = vbByteSize;
	bufferview.push_back({ VerPosBufferGPU->GetGPUVirtualAddress(), VertexBufferByteSize, VertexByteStride });

	VertexByteStride = sizeof(VColorData);
	VertexBufferByteSize = vColorByteSize;
	bufferview.push_back({ VerColorBufferGPU->GetGPUVirtualAddress(), VertexBufferByteSize, VertexByteStride });
	//************************Chapter 6 exercise 2**********************************************************************

	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}
