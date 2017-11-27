/*
Construct the vertex and index list of a pyramid, as shown in Figure 6.8, and draw it. 
Color the base vertices green and the tip vertex red.
*/

...

void BoxApp::BuildBoxGeometry()
{
//**********************Changed********************************
    std::array<Vertex, 5> vertices =
    {
		Vertex{XMFLOAT3(.0f,1.0f,.0f),XMFLOAT4(Colors::Red)},
		Vertex{XMFLOAT3(.0f,.0f,1.0f),XMFLOAT4(Colors::Green)},
		Vertex{XMFLOAT3(.0f,.0f,-1.0f),XMFLOAT4(Colors::Green)},
		Vertex{ XMFLOAT3(1.0f,.0f,.0f),XMFLOAT4(Colors::Green) },
		Vertex{ XMFLOAT3(-1.0f,.0f,.0f),XMFLOAT4(Colors::Green) },
    };

	std::array<std::uint16_t, 36> indices =
	{
		//front face
		4,0,2,
		2,0,3,
		3,0,1,
		2,0,1,
		//down face
		2,4,1,
		1,3,2
	};
//**********************Changed********************************

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}
