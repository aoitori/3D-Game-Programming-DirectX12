# 3D-Game-Programming-DirectX12
Exercises answers

Chapter 6 exercise 6

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	  float gTime = 0.0f; //Added
};

void BoxApp::Update(const GameTimer& gt)
{
...
    objConstants.gTime = gt.TotalTime(); //Added
    mObjectCB->CopyData(0, objConstants);
}
