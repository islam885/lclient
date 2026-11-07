#pragma once

#include <game/client/component.h>
#include <game/client/gameclient.h>

class CLClient : public CComponent
{
public:
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnRender() override;
	void OnSnapInput(int *pData);

	// Aimbot logic
	void DoAimbot();
	void HookVisible(vec2 TargetPos);
	void Aim(vec2 Pos);
	vec2 NormalizeAim(vec2 Pos);
	int GetClosestId(int Fov, float Range = 395.0f);
	vec2 EdgeScan();
	bool InFov(float Fov, vec2 Dir);
	bool PredictHook(vec2 &MyPos, vec2 MyVel, vec2 &TargetPos, vec2 TargetVel);
	bool HitScanHook(vec2 InitPos, vec2 TargetPos, vec2 ScanDir);
	bool IntersectCharacter(vec2 HookPos, vec2 TargetPos, vec2 &NewPos);
	float GetPing() const;

	// Visuals / ESP
	void RenderEsp();
	void DrawLine(vec2 Pos1, vec2 Pos2, ColorRGBA Color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
	void DrawFov();
	void DrawFovLine(float Offset);

	// State
	bool m_CanAim = true;
	bool m_TargetVisible = false;
	int m_TargetId = -1;
	vec2 m_MyPos = vec2(0.0f, 0.0f);
	vec2 m_MyVel = vec2(0.0f, 0.0f);
	vec2 m_TargetPos = vec2(0.0f, 0.0f);
	vec2 m_TargetVel = vec2(0.0f, 0.0f);
};

