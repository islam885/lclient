#include "menus.h"

#include <engine/shared/config.h>
#include <game/client/ui.h>
#include <game/localization.h>

void CMenus::RenderSettingsLClient(CUIRect MainView)
{
	CUIRect AimbotSection, EspSection, Label, Left, Right, Button;

	// Title / Aimbot section
	MainView.HSplitTop(220.0f, &AimbotSection, &MainView);
	AimbotSection.HSplitTop(30.0f, &Label, &AimbotSection);
	Ui()->DoLabel(&Label, "LClient Aimbot", 20.0f, TEXTALIGN_ML);

	AimbotSection.HSplitTop(10.0f, nullptr, &AimbotSection);
	AimbotSection.VSplitMid(&Left, &Right, 20.0f);

	// Aimbot Enabled Checkbox
	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_LClientAimbot, Localize("Enable Aimbot"), g_Config.m_LClientAimbot, &Button))
	{
		g_Config.m_LClientAimbot ^= 1;
	}

	// Silent Aim Checkbox
	Left.HSplitTop(10.0f, nullptr, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_LClientAimbotSilent, Localize("Silent Aim"), g_Config.m_LClientAimbotSilent, &Button))
	{
		g_Config.m_LClientAimbotSilent ^= 1;
	}

	// Auto Hook When Visible Checkbox
	Left.HSplitTop(10.0f, nullptr, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_LClientAimbotHookVisible, Localize("Auto-Hook when visible"), g_Config.m_LClientAimbotHookVisible, &Button))
	{
		g_Config.m_LClientAimbotHookVisible ^= 1;
	}

	// Edge Scan Checkbox
	Left.HSplitTop(10.0f, nullptr, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_LClientAimbotEdgeScan, Localize("Edge Scan"), g_Config.m_LClientAimbotEdgeScan, &Button))
	{
		g_Config.m_LClientAimbotEdgeScan ^= 1;
	}

	// FOV Slider
	Right.HSplitTop(20.0f, &Button, &Right);
	Ui()->DoScrollbarOption(&g_Config.m_LClientAimbotFov, &g_Config.m_LClientAimbotFov, &Button, Localize("FOV Angle"), 1, 360, &CUi::ms_LinearScrollbarScale, 0u, "°");

	// Accuracy Slider
	Right.HSplitTop(10.0f, nullptr, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	Ui()->DoScrollbarOption(&g_Config.m_LClientAimbotAccuracy, &g_Config.m_LClientAimbotAccuracy, &Button, Localize("Edge Scan Accuracy"), 1, 50, &CUi::ms_LinearScrollbarScale);

	// Visuals / ESP section
	MainView.HSplitTop(15.0f, nullptr, &MainView);
	MainView.HSplitTop(150.0f, &EspSection, &MainView);
	EspSection.HSplitTop(30.0f, &Label, &EspSection);
	Ui()->DoLabel(&Label, "LClient Visuals & ESP", 20.0f, TEXTALIGN_ML);

	EspSection.HSplitTop(10.0f, nullptr, &EspSection);
	EspSection.VSplitMid(&Left, &Right, 20.0f);

	// ESP Enabled Checkbox
	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_LClientEsp, Localize("Enable Visuals"), g_Config.m_LClientEsp, &Button))
	{
		g_Config.m_LClientEsp ^= 1;
	}

	// Draw FOV Lines Checkbox
	Left.HSplitTop(10.0f, nullptr, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_LClientEspDrawFov, Localize("Draw FOV Lines"), g_Config.m_LClientEspDrawFov, &Button))
	{
		g_Config.m_LClientEspDrawFov ^= 1;
	}
}

