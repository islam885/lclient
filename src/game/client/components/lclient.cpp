#include "lclient.h"

#include <base/math.h>
#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>

#include <algorithm>
#include <cmath>

static constexpr float LCLIENT_PHYS_SIZE = 28.0f;
static constexpr int LCLIENT_MAX_HITPOINTS = 32;

void CLClient::OnSnapInput(int *pData)
{
	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	DoAimbot();
}

void CLClient::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	RenderEsp();
}

float CLClient::GetPing() const
{
	const float RealPing = static_cast<float>(Client()->GetPredictionTime());
	return RealPing / 100.0f;
}

bool CLClient::InFov(float Fov, vec2 Dir)
{
	const int Dummy = g_Config.m_ClDummy;
	const float DiffAngle = std::abs(std::atan2(std::sin(angle(Dir) - angle(GameClient()->m_Controls.m_aMousePos[Dummy])),
		std::cos(angle(Dir) - angle(GameClient()->m_Controls.m_aMousePos[Dummy])))) * 100.0f;
	return DiffAngle <= Fov;
}

int CLClient::GetClosestId(int Fov, float Range)
{
	const vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
	float Distance = Range;
	int ClosestID = -1;

	const int Local = g_Config.m_ClDummy;
	const int LocalId = GameClient()->m_aLocalIds[Local];
	if(LocalId < 0 || LocalId >= MAX_CLIENTS)
		return -1;

	const CGameClient::CClientData OwnClientData = GameClient()->m_aClients[LocalId];

	auto *pPlayer = dynamic_cast<CCharacter *>(GameClient()->m_GameWorld.FindFirst(GameClient()->m_GameWorld.ENTTYPE_CHARACTER));
	for(; pPlayer; pPlayer = dynamic_cast<CCharacter *>(pPlayer->TypeNext()))
	{
		int i = pPlayer->GetId();
		if(i == LocalId || !pPlayer)
			continue;

		const CGameClient::CClientData &cData = GameClient()->m_aClients[i];
		if(!cData.m_Active)
			continue;

		vec2 Position = GameClient()->m_aClients[i].m_Predicted.m_Pos;

		const bool IsOneSolo = cData.m_Solo || OwnClientData.m_Solo;
		const bool IsOneSpec = cData.m_Spec || OwnClientData.m_Spec;
		if(IsOneSpec || IsOneSolo)
			continue;

		if(!GameClient()->m_Teams.SameTeam(i, LocalId) || OwnClientData.m_HookHitDisabled)
			continue;

		if(!InFov(static_cast<float>(Fov), Position - Pos))
			continue;

		if(ClosestID != -1 && GameClient()->m_GameWorld.m_GameTick % 150 != 0)
			return ClosestID;

		if(ClosestID == -1 && distance(Pos, Position) < Distance)
		{
			ClosestID = i;
			Distance = distance(Pos, Position);
		}
	}
	return ClosestID;
}

bool CLClient::PredictHook(vec2 &MyPos, vec2 MyVel, vec2 &TargetPos, vec2 TargetVel)
{
	const vec2 Delta = TargetPos - MyPos;
	const vec2 DeltaVel = TargetVel - MyVel;

	const float HookSpeed = length(TargetVel) + GameClient()->m_aTuning[g_Config.m_ClDummy].m_HookFireSpeed;
	const float a = dot(DeltaVel, DeltaVel) - HookSpeed * HookSpeed;
	const float b = 2.0f * dot(DeltaVel, Delta);
	const float c = dot(Delta, Delta);

	const float Sol = b * b - 4.0f * a * c;
	if(Sol > 0.0f)
	{
		const float Time = std::abs(2.0f * c / (std::sqrt(Sol) - b)) + GetPing();
		TargetPos += TargetVel * Time;
		return true;
	}
	return false;
}

bool CLClient::IntersectCharacter(vec2 HookPos, vec2 TargetPos, vec2 &NewPos)
{
	vec2 ClosestPoint;
	if(closest_point_on_line(HookPos, NewPos, TargetPos, ClosestPoint))
	{
		if(distance(TargetPos, ClosestPoint) < LCLIENT_PHYS_SIZE + 2.0f)
		{
			NewPos = ClosestPoint;
			return true;
		}
	}
	return false;
}

bool CLClient::HitScanHook(vec2 InitPos, vec2 TargetPos, vec2 ScanDir)
{
	if(length(ScanDir) < 0.001f)
		return false;

	vec2 ExDirection = normalize(ScanDir);
	const float HookLength = GameClient()->m_aTuning[g_Config.m_ClDummy].m_HookLength;
	const float HookFireSpeed = GameClient()->m_aTuning[g_Config.m_ClDummy].m_HookFireSpeed;

	vec2 FinishPos = InitPos + ExDirection * (HookLength - LCLIENT_PHYS_SIZE * 1.5f);
	vec2 OldPos = InitPos + ExDirection * LCLIENT_PHYS_SIZE * 1.5f;
	vec2 NewPos = OldPos;

	bool DoBreak = false;

	do
	{
		OldPos = NewPos;
		NewPos = OldPos + ExDirection * HookFireSpeed;

		if(distance(InitPos, NewPos) > HookLength)
		{
			NewPos = InitPos + normalize(NewPos - InitPos) * HookLength;
			DoBreak = true;
		}

		int TeleNr = 0;
		const int Hit = Collision()->IntersectLineTeleHook(OldPos, NewPos, &FinishPos, nullptr, &TeleNr);

		if(IntersectCharacter(OldPos, TargetPos, FinishPos))
			return true;

		if(Hit)
			break;

		NewPos.x = round_to_int(NewPos.x);
		NewPos.y = round_to_int(NewPos.y);

		if(OldPos == NewPos)
			break;

		ExDirection.x = round_to_int(ExDirection.x * 256.0f) / 256.0f;
		ExDirection.y = round_to_int(ExDirection.y * 256.0f) / 256.0f;
	} while(!DoBreak);

	return false;
}

vec2 CLClient::EdgeScan()
{
	int HitPointsCount = 0;
	vec2 aHitPoints[LCLIENT_MAX_HITPOINTS];

	vec2 MyPos = m_MyPos;
	vec2 TargetPos = m_TargetPos;

	m_TargetVisible = false;

	if(!PredictHook(MyPos, m_MyVel, TargetPos, m_TargetVel))
		return vec2(0.0f, 0.0f);

	if(HitScanHook(MyPos, TargetPos, TargetPos - MyPos))
	{
		m_TargetVisible = true;
		return TargetPos - MyPos;
	}

	if(!g_Config.m_LClientAimbotEdgeScan)
		return vec2(0.0f, 0.0f);

	const float VisibleAngle = std::atan2(TargetPos.y - MyPos.y, TargetPos.x - MyPos.x) + pi * 0.5f;
	const float Step = 1.0f / std::max(1.0f, static_cast<float>(g_Config.m_LClientAimbotAccuracy));

	for(float i = VisibleAngle; i < pi + VisibleAngle; i += Step)
	{
		if(HitPointsCount >= LCLIENT_MAX_HITPOINTS)
			break;

		auto Pos = vec2(static_cast<float>(static_cast<int>(TargetPos.x + std::cos(i) * LCLIENT_PHYS_SIZE)),
			static_cast<float>(static_cast<int>(TargetPos.y + std::sin(i) * LCLIENT_PHYS_SIZE)));
		const vec2 Dir = Pos - MyPos;

		if(HitScanHook(MyPos, TargetPos, Dir))
		{
			aHitPoints[HitPointsCount] = Dir;
			HitPointsCount++;
		}
	}

	if(HitPointsCount > 0)
	{
		const int MiddleIndex = (HitPointsCount - 1) / 2;
		m_TargetVisible = true;
		m_TargetPos = aHitPoints[MiddleIndex];
		return aHitPoints[MiddleIndex];
	}

	return vec2(0.0f, 0.0f);
}

vec2 CLClient::NormalizeAim(vec2 Pos)
{
	constexpr float CameraMaxDistance = 200.0f;
	const float FollowFactor = (g_Config.m_ClDyncam ? g_Config.m_ClDyncamFollowFactor : g_Config.m_ClMouseFollowfactor) / 100.0f;
	const float DeadZone = g_Config.m_ClDyncam ? g_Config.m_ClDyncamDeadzone : g_Config.m_ClMouseDeadzone;
	const float MaxDistance = static_cast<float>(g_Config.m_ClMouseMaxDistance);
	const float MouseMax = std::min((FollowFactor != 0.0f ? CameraMaxDistance / FollowFactor + DeadZone : MaxDistance), MaxDistance);
	const float MDistance = length(Pos);
	Pos = normalize_pre_length(Pos, MDistance) * MouseMax;
	Pos = vec2(static_cast<float>(static_cast<int>(Pos.x)), static_cast<float>(static_cast<int>(Pos.y)));
	return Pos;
}

void CLClient::Aim(vec2 Pos)
{
	if(!m_CanAim || !m_TargetVisible)
		return;

	m_CanAim = false;
	const int Local = g_Config.m_ClDummy;

	if(!g_Config.m_LClientAimbotSilent)
	{
		GameClient()->m_Controls.m_aMousePos[Local] = Pos;
		GameClient()->m_Controls.m_aInputData[Local].m_TargetX = static_cast<int>(GameClient()->m_Controls.m_aMousePos[Local].x);
		GameClient()->m_Controls.m_aInputData[Local].m_TargetY = static_cast<int>(GameClient()->m_Controls.m_aMousePos[Local].y);
	}
	else
	{
		GameClient()->m_Controls.m_aInputData[Local].m_TargetX = static_cast<int>(Pos.x);
		GameClient()->m_Controls.m_aInputData[Local].m_TargetY = static_cast<int>(Pos.y);
	}
}

void CLClient::HookVisible(vec2 TargetPos)
{
	static bool s_HasHooked = false;
	const int Local = g_Config.m_ClDummy;

	if(!g_Config.m_LClientAimbotHookVisible)
	{
		if(s_HasHooked)
		{
			GameClient()->m_Controls.m_aLastData[Local].m_Hook = 0;
			GameClient()->m_Controls.m_aInputData[Local].m_Hook = GameClient()->m_Controls.m_aLastData[Local].m_Hook;
			s_HasHooked = false;
		}
		return;
	}

	if(!m_TargetVisible)
		return;

	m_CanAim = !s_HasHooked;

	Aim(NormalizeAim(TargetPos));
	GameClient()->m_Controls.m_aInputData[Local].m_Hook = 1;
	s_HasHooked = true;
}

void CLClient::DoAimbot()
{
	if(!g_Config.m_LClientAimbot)
		return;

	const int Local = g_Config.m_ClDummy;

	m_TargetId = GetClosestId(g_Config.m_LClientAimbotFov);
	if(m_TargetId < 0 || m_TargetId >= MAX_CLIENTS)
	{
		m_TargetPos = vec2(0.0f, 0.0f);
		m_TargetVel = vec2(0.0f, 0.0f);
		m_TargetVisible = false;
		m_TargetId = -1;
		m_CanAim = true;
		return;
	}

	m_MyPos = GameClient()->m_PredictedChar.m_Pos;
	m_MyVel = GameClient()->m_PredictedChar.m_Vel;
	m_TargetPos = GameClient()->m_aClients[m_TargetId].m_Predicted.m_Pos;
	m_TargetVel = GameClient()->m_aClients[m_TargetId].m_Predicted.m_Vel;
	m_TargetPos = EdgeScan();

	HookVisible(m_TargetPos);

	if(GameClient()->m_Controls.m_aInputData[Local].m_Hook == 1)
		Aim(NormalizeAim(m_TargetPos));
	else
		m_CanAim = true;
}

void CLClient::DrawLine(vec2 Pos1, vec2 Pos2, ColorRGBA Color)
{
	Graphics()->LinesBegin();
	const IGraphics::CLineItem LineItem(Pos1.x, Pos1.y, Pos2.x, Pos2.y);
	Graphics()->SetColor(Color);
	Graphics()->LinesDraw(&LineItem, 1);
	Graphics()->LinesEnd();
}

void CLClient::DrawFovLine(float Offset)
{
	const int Dummy = g_Config.m_ClDummy;
	const float Angle = angle(GameClient()->m_Controls.m_aMousePos[Dummy]) + Offset;
	const vec2 ExDirection = normalize(direction(Angle));

	const vec2 InitPos = GameClient()->m_LocalCharacterPos;
	vec2 FinishPos = InitPos + ExDirection * (GameClient()->m_aTuning[Dummy].m_HookLength);

	int TeleNr = 0;
	Collision()->IntersectLineTeleHook(InitPos, FinishPos, &FinishPos, nullptr, &TeleNr);

	const ColorRGBA HookCollColor(0.0f, 0.8f, 1.0f, 0.6f);
	DrawLine(InitPos, FinishPos, HookCollColor);
}

void CLClient::DrawFov()
{
	if(!g_Config.m_LClientEspDrawFov)
		return;

	DrawFovLine(static_cast<float>(g_Config.m_LClientAimbotFov) * 0.01f);
	DrawFovLine(static_cast<float>(g_Config.m_LClientAimbotFov) * -0.01f);
}

void CLClient::RenderEsp()
{
	if(!g_Config.m_LClientEsp)
		return;

	Graphics()->TextureClear();
	DrawFov();
}

