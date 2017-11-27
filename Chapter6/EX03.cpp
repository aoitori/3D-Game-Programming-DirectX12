/*
3.Draw 
  1.a point list like the one shown in Figure 5.13a.
  2.a line strip like the one shown in Figure 5.13b.
  3.a line list like the one shown in Figure 5.13c.
  4.a triangle strip like the one shown in Figure 5.13d.
  5.a triangle list like the one shown in Figure 5.14a.
*/
...

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
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Black, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
    // Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
	//mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
	
//****************************Changed********************************************	
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
 //↑Change Primitive Topology to POINTLIST(Q1), LINESTRIP(Q2), LINELIST(Q3), TIRANGLESTRIP(Q4),TIRANGLELIST(Q5).
    
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    mCommandList->DrawInstanced(8,8,0,0);//mCommandList->DrawInstanced(9,9,0,0); (Question 5)
//****************************Changed********************************************	
	
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
//****************************Changed********************************************
    std::array<Vertex, 8> vertices =
    {
		Vertex{XMFLOAT3{-1.5f, -0.4f, 0.0f},XMFLOAT4{Colors::GhostWhite}},
		Vertex{ XMFLOAT3{ -1.0f, +0.5f, 0.0f },XMFLOAT4{ Colors::Red } },
		Vertex{ XMFLOAT3{ -0.5f, -0.1f, 0.0f },XMFLOAT4{ Colors::Green } },
		Vertex{ XMFLOAT3{ +0.0f, 0.3f, 0.0f },XMFLOAT4{ Colors::Yellow } },
		Vertex{ XMFLOAT3{ +0.5f, 0.0f, 0.0f },XMFLOAT4{ Colors::White } },
		Vertex{ XMFLOAT3{ +1.0f, 0.4f, 0.0f },XMFLOAT4{ Colors::Pink } },
		Vertex{ XMFLOAT3{ +1.5f, +0.1f, 0.0f },XMFLOAT4{ Colors::Magenta } },
		Vertex{ XMFLOAT3{ +2.0f, 0.5f, 0.0f },XMFLOAT4{ Colors::Cyan } },
    };
    
	/* Question 5.
	std::array<Vertex, 9> vertices =
    {
		Vertex{XMFLOAT3{-1.5f, -0.4f, 0.0f},XMFLOAT4{Colors::GhostWhite}},
		Vertex{ XMFLOAT3{ -1.0f, +0.5f, 0.0f },XMFLOAT4{ Colors::Red } },
		Vertex{ XMFLOAT3{ -0.5f, -0.1f, 0.0f },XMFLOAT4{ Colors::Green } },

		Vertex{ XMFLOAT3{ +0.0f, 0.3f, 0.0f },XMFLOAT4{ Colors::Yellow } },
		Vertex{ XMFLOAT3{ +1.0f, 0.4f, 0.0f },XMFLOAT4{ Colors::Pink } },
		Vertex{ XMFLOAT3{ +0.5f, 0.0f, 0.0f },XMFLOAT4{ Colors::White } },

		Vertex{ XMFLOAT3{ +1.5f, +0.1f, 0.0f },XMFLOAT4{ Colors::Magenta } },
		Vertex{ XMFLOAT3{ +2.0f, 0.5f, 0.0f },XMFLOAT4{ Colors::Cyan } },
		Vertex{ XMFLOAT3{ +2.5f, 0.2f, 0.0f },XMFLOAT4{ Colors::Snow} },
    };
   */
//****************************Changed********************************************
    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	//const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	/*
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	*/
	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);
	/*
	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);
		*/
	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	//mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)0;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}
