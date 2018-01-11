
#include "SkyXEngine.h"

//##########################################################################

//! поток ведения лога
FILE *g_pFileOutLog = 0;	

BOOL CALLBACK SkyXEngine_EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == lParam && IsWindow(hwnd))
		SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	return TRUE;
}

void SkyXEngine_HandlerError(const char *szFormat, ...)
{
	va_list va;
	char buf[4096];
	va_start(va, szFormat);
	vsprintf_s(buf, 4096, szFormat, va);
	va_end(va);

	DWORD PID = GetCurrentProcessId();
	EnumWindows(SkyXEngine_EnumWindowsProc, PID);

	MessageBox(0, buf, "SkyXEngine error", MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);
	exit(0);
}

void SkyXEngine_InitOutLog()
{
	AllocConsole();
	freopen("CONOUT$", "wt", stdout);
	freopen("CONOUT$", "wt", stderr);
	freopen("CONIN$", "rt", stdin);

	char path[256];
	char PathForExe[1024];
	GetModuleFileName(NULL, path, 256);
	int len = strlen(path);
	while (path[len--] != '\\')
	{
		if (path[len - 1] == '\\')
		{
			len--;
			memcpy(PathForExe, path, len);
			PathForExe[len] = 0;
		}
	}
	char FileLogPath[1024];
	sprintf(FileLogPath, "%s%s", PathForExe, "/log.txt");
	g_pFileOutLog = fopen(FileLogPath, "w");

	if (!g_pFileOutLog)
	{
		SkyXEngine_HandlerError("Debug system is not init, this is big problem!");
		exit(0);
	}
}

void SkyXEngine_PrintfLog(int level, const char *szFormat, ...)
{
	va_list va;
	char buf[REPORT_MSG_MAX_LEN];
	va_start(va, szFormat);
	vsprintf_s(buf, REPORT_MSG_MAX_LEN, szFormat, va);
	va_end(va);

	if (g_pFileOutLog)
	{
		if (level == REPORT_MSG_LEVEL_ERROR)
		{
			printf(COLOR_LRED "! ");
			fwrite("! ", 1, 2, g_pFileOutLog);
		}
		else if (level == REPORT_MSG_LEVEL_WARNING)
		{
			printf(COLOR_YELLOW "* ");
			fwrite("* ", 1, 2, g_pFileOutLog);
		}

		printf(buf);
		if (level == REPORT_MSG_LEVEL_ERROR || level == REPORT_MSG_LEVEL_WARNING)
		{
			printf(COLOR_RESET);
		}
		fwrite(buf, 1, strlen(buf), g_pFileOutLog);
		//fprintf(FileOutLog, "\n");
		fflush(g_pFileOutLog);

		if (level == REPORT_MSG_LEVEL_ERROR)
		{
			SkyXEngine_HandlerError(buf);
		}
	}
}

//##########################################################################

void SkyXEngine_Init(HWND hWnd3D, HWND hWndParent3D)
{
	srand((UINT)time(0));
	SkyXEngine_InitOutLog();
	SkyXEngine_InitPaths();

	if (!Core_0IsProcessRun("sxconsole.exe"))
		ShellExecute(0, "open", "sxconsole.exe", 0, Core_RStringGet(G_RI_STRING_PATH_EXE), SW_SHOWNORMAL);

	

	Core_0Create("sxcore", false);
	Core_Dbg_Set(SkyXEngine_PrintfLog);
	Core_SetOutPtr();

	SkyXEngine_CreateLoadCVar();

	ID idTimerRender = Core_TimeAdd();
	ID idTimerGame = Core_TimeAdd();
	Core_RIntSet(G_RI_INT_TIMER_RENDER, idTimerRender);
	Core_RIntSet(G_RI_INT_TIMER_GAME, idTimerGame);

	tm ct = { 0, 0, 10, 27, 5, 2030 - 1900, 0, 0, 0 };
	Core_TimeUnixStartSet(idTimerGame, mktime(&ct));

	Core_TimeWorkingSet(idTimerRender, true);
	Core_TimeWorkingSet(idTimerGame, true);

	Core_TimeSpeedSet(idTimerGame, 10);

	

	static int *r_win_width = (int*)GET_PCVAR_INT("r_win_width");
	static int *r_win_height = (int*)GET_PCVAR_INT("r_win_height");
	static const bool *r_win_windowed = GET_PCVAR_BOOL("r_win_windowed");

	HWND hWnd3DCurr = 0;

	if (hWnd3D == 0)
		hWnd3DCurr = SkyXEngine_CreateWindow("SkyXEngine", "SkyXEngine", *r_win_width, *r_win_height);
	else
	{
		hWnd3DCurr = hWnd3D;

		RECT rect;
		GetClientRect(hWnd3DCurr, &rect);

		*r_win_width = rect.right;
		*r_win_height = rect.bottom;
	}
	
	SSInput_0Create("sxinput", hWnd3DCurr, false);
	SSInput_Dbg_Set(SkyXEngine_PrintfLog);

	SSCore_0Create("sxsound", hWnd3DCurr, false);
	SSCore_Dbg_Set(SkyXEngine_PrintfLog);

	SGCore_0Create("sxgcore", hWnd3DCurr, *r_win_width, *r_win_height, *r_win_windowed, 0, false);
	SGCore_Dbg_Set(SkyXEngine_PrintfLog);

	SGCore_SetFunc_MtlSet(SkyXEngine_RFuncMtlSet);
	SGCore_SetFunc_MtlLoad(SkyXEngine_RFuncMtlLoad);
	SGCore_SetFunc_MtlGetSort((g_func_mtl_get_sort)SML_MtlGetTypeTransparency);
	SGCore_SetFunc_MtlGroupRenderIsSingly((g_func_mtl_group_render_is_singly)SML_MtlGetTypeReflection);
	SGCore_SetFunc_MtlGetPhysicType((g_func_mtl_get_physic_type)SML_MtlGetPhysicMaterial);

	SGCore_SkyBoxCr();
	SGCore_SkyCloudsCr();

	SGCore_GetDXDevice()->SetRenderState(D3DRS_LIGHTING, FALSE);

	SGeom_0Create("sxgeom", false);
	SGeom_Dbg_Set(SkyXEngine_PrintfLog);

	SML_0Create("sxml", false);
	SML_Dbg_Set(SkyXEngine_PrintfLog);

	SPE_0Create("sxparticles", false);
	SPE_Dbg_Set(SkyXEngine_PrintfLog);
	SPE_SetFunc_ParticlesPhyCollision(SkyXEngine_RFuncParticlesPhyCollision);
	SPE_RTDepthSet(SML_DSGetRT_ID(DS_RT_DEPTH));


	SPP_0Create("sxpp", false);
	SPP_Dbg_Set(SkyXEngine_PrintfLog);

#if defined(SX_GAME)
	SPP_ChangeTexSun("fx_sun.dds");
#endif
	SPP_RTSetInput(SML_DSGetRT_ID(DS_RT_SCENELIGHT));
	SPP_RTSetOutput(SML_DSGetRT_ID(DS_RT_SCENELIGHT2));
	SPP_RTSetDepth0(SML_DSGetRT_ID(DS_RT_DEPTH0));
	SPP_RTSetDepth1(SML_DSGetRT_ID(DS_RT_DEPTH1));
	SPP_RTSetNormal(SML_DSGetRT_ID(DS_RT_NORMAL));

	SXAnim_0Create();
	SXAnim_Dbg_Set(SkyXEngine_PrintfLog);

	SXPhysics_0Create();
	SXPhysics_Dbg_Set(SkyXEngine_PrintfLog);

	SXDecals_0Create();
	SXDecals_Dbg_Set(SkyXEngine_PrintfLog);

#if defined(SX_LEVEL_EDITOR)
	SAIG_0Create("sxaigrid", true, false);
	SAIG_BBCreate(&float3(0, 0, 0), &float3(10, 10, 10));
#else
	SAIG_0Create("sxaigrid", true, false);
#endif
	SAIG_Dbg_Set(SkyXEngine_PrintfLog);
	SAIG_SetFunc_QuadPhyNavigate(SkyXEngine_RFuncAIQuadPhyNavigate);


	SLevel_0Create("sxlevel", false);
	SLevel_Dbg_Set(SkyXEngine_PrintfLog);


#ifndef SX_PARTICLES_EDITOR
	SXGame_0Create();
	SXGame_Dbg_Set(SkyXEngine_PrintfLog);
#endif














	

	SRender_0Create("sxrender", hWnd3DCurr, hWndParent3D, false);
	SRender_Dbg_Set(SkyXEngine_PrintfLog);

	


#ifndef SX_GAME
	ISXCamera *pCamera = SGCore_CrCamera();
	static const float *r_default_fov = GET_PCVAR_FLOAT("r_default_fov");
	pCamera->SetFOV(*r_default_fov);

	SRender_SetCamera(pCamera);
#endif

#ifdef SX_GAME
	SRender_SetCamera(SXGame_GetActiveCamera());
#endif

#if !defined(SX_GAME)
	Core_0ConsoleExecCmd("exec ../editor.cfg");
#endif

#if defined(SX_GAME)
	if (*r_win_windowed)
		ShowWindow(hWnd3DCurr, SW_SHOW);
	else
		ShowWindow(hWnd3DCurr, SW_MAXIMIZE);
#endif
}

//**************************************************************************

void SkyXEngine_InitPaths()
{
	char tmppath[MAX_PATH];
	char tmppathexe[MAX_PATH];
	GetModuleFileName(NULL, tmppath, MAX_PATH);
	int len = strlen(tmppath);
	while (tmppath[len--] != '\\')
	{
		if (tmppath[len - 1] == '\\')
		{
			len--;
			memcpy(tmppathexe, tmppath, len);
			tmppathexe[len] = 0;
		}
	}

	Core_RStringSet(G_RI_STRING_PATH_EXE, tmppathexe);

	sprintf(tmppath, "%s%s", tmppathexe, "/worktex/");
	Core_RStringSet(G_RI_STRING_PATH_WORKTEX, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/");
	Core_RStringSet(G_RI_STRING_PATH_GAMESOURCE, tmppath);
	SetCurrentDirectoryA(tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/screenshots/");
	Core_RStringSet(G_RI_STRING_PATH_SCREENSHOTS, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/config/");
	Core_RStringSet(G_RI_STRING_PATH_GS_CONFIGS, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/levels/");
	Core_RStringSet(G_RI_STRING_PATH_GS_LEVELS, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/meshes/");
	Core_RStringSet(G_RI_STRING_PATH_GS_MESHES, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/models/");
	Core_RStringSet(G_RI_STRING_PATH_GS_MODELS, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/shaders/");
	Core_RStringSet(G_RI_STRING_PATH_GS_SHADERS, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/sounds/");
	Core_RStringSet(G_RI_STRING_PATH_GS_SOUNDS, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/scripts/");
	Core_RStringSet(G_RI_STRING_PATH_GS_SCRIPTS, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/textures/");
	Core_RStringSet(G_RI_STRING_PATH_GS_TEXTURES, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/materials/");
	Core_RStringSet(G_RI_STRING_PATH_GS_MTRLS, tmppath);

	sprintf(tmppath, "%s%s", tmppathexe, "/gamesource/resource/");
	Core_RStringSet(G_RI_STRING_PATH_GS_GUI, tmppath);
}

void SkyXEngine_CreateLoadCVar()
{
	Core_0RegisterCVarInt("r_win_width", 800, "Размер окна по горизонтали (в пикселях)");
	Core_0RegisterCVarInt("r_win_height", 600, "Размер окна по вертикали (в пикселях)");
	Core_0RegisterCVarBool("r_win_windowed", true, "Режим рендера true - оконный, false - полноэкранный");
	Core_0RegisterCVarFloat("r_default_fov", SM_PI * 0.25f, "Дефолтный fov в радианах");
	Core_0RegisterCVarFloat("r_near", 0.025f, "Ближняя плоскость отсчечения");
	Core_0RegisterCVarFloat("r_far", 400, "Дальняя плоскость отсечения (дальность видимости)");

	Core_0RegisterCVarInt("r_final_image", DS_RT_SCENELIGHT, "Тип финального (выводимого в окно рендера) изображения из перечисления DS_RT");

	Core_0RegisterCVarInt("r_resize", RENDER_RESIZE_NONE, "Тип изменения размеров окан рендера из перечисления RENDER_RESIZE");


	Core_0RegisterCVarBool("r_pssm_4or3", true, "Для глобальных теней использовать 4 (true) или 3 (false) сплита?");
	Core_0RegisterCVarBool("r_pssm_shadowed", true, "Глобальный источник отбрасывает тени?");
	Core_0RegisterCVarFloat("r_pssm_quality", 1, "Коэфициент размера карты глубины глобального источника света относительно размеров окна рендера [0.5,4] (низкое, высокое)");
	Core_0RegisterCVarFloat("r_lsm_quality", 1, "Коэфициент размера карты глубины для локальных источников света относительно размеров окна рендера [0.5,4] (низкое, высокое)");
	Core_0RegisterCVarInt("r_shadow_soft", 1, "Дополнительнео размытие теней, 0 - нет, 1 - 1 проход, 2 - 2 прохода");
	Core_0RegisterCVarFloat("r_hdr_adapted_coef", 0.03f, "Коэфициент привыкания к освещению (0,1] (медлено, быстро)");

	Core_0RegisterCVarInt("r_grass_freq", 100, "Плотность отрисовки травы [1,100]");
	Core_0RegisterCVarFloat("r_green_lod0", 50, "Дистанция отрисовки для первого лода (он же лод травы) растительности, начиная с нуля от камеры");
	Core_0RegisterCVarFloat("r_green_lod1", 100, "Дистанция отрисовки второго лода растительности (кусты/деревья), старт с дистанции первого лода");
	Core_0RegisterCVarFloat("r_green_less", 20, "Дистанция? после которой трава будет уменьшаться (0,r_green_lod0)");

	Core_0RegisterCVarInt("r_texfilter_type", 2, "Тип фильтрации текстур, 0 - точечная, 1 - линейная, 2 - анизотропная");
	Core_0RegisterCVarInt("r_texfilter_max_anisotropy", 16, "Максимальное значение анизотропной фильтрации (если включена) [1,16]");
	Core_0RegisterCVarInt("r_texfilter_max_miplevel", 0, "Какой mip уровень текстур использовать? 0 - самый высокий, 1 - ниже на один уровень и т.д.");
	Core_0RegisterCVarInt("r_stats", 1, "Показывать ли статистику? 0 - нет, 1 - fps и игровое время, 2 - показать полностью");


	Core_0RegisterCVarFloat("cl_mousesense", 0.001f, "Mouse sense value");

	Core_0RegisterCVarInt("pp_ssao", 1, "Рисовать ли эффект ssao? 0 - нет, 1 - на низком качестве, 2 - на среднем, 3 - на высоком");
	Core_0RegisterCVarBool("pp_bloom", true, "Рисовать ли эффект bloom?");
	Core_0RegisterCVarBool("pp_lensflare", true, "Рисовать ли эффект lens flare?");
	Core_0RegisterCVarBool("pp_lensflare_usebloom", true, "При отрисовке эффекта lens flare использовать ли данные от прохода bloom?");
	Core_0RegisterCVarBool("pp_dlaa", true, "Рисовать ли эффект dlaa?");
	Core_0RegisterCVarBool("pp_nfaa", false, "Рисовать ли эффект nfaa?");

	static float3_t fog_color(0.5, 0.5, 0.5);
	Core_0RegisterCVarPointer("pp_fog_color", ((UINT_PTR)&fog_color));
	Core_0RegisterCVarFloat("pp_fog_density", 0.5, "Плотность тумана");

	Core_0RegisterCVarBool("pp_motionblur", true, "Рисовать ли эффект motion blur?");
	Core_0RegisterCVarFloat("pp_motionblur_coef", 0.1, "Коэфициент для эффекта motion blur [0,1]");
	
	Core_0RegisterCVarBool("g_time_run", true, "Запущено ли игрвоое время?");
	Core_0RegisterCVarFloat("g_time_speed", 1.f, "Скорость/соотношение течения игрового времени");

	Core_0RegisterCVarFloat("env_default_rain_density", 1.f, "Коэфициент плотности дождя (0,1]");
	Core_0RegisterCVarBool("env_default_thunderbolt", true, "Могут ли воспроизводится эффекты молнии?");

	Core_0RegisterCVarFloat("env_weather_snd_volume", 1.f, "Громкость звуков погоды [0,1]");
	Core_0RegisterCVarFloat("env_ambient_snd_volume", 1.f, "Громкость фоновых звуков на уровне [0,1]");

	Core_0RegisterConcmd("screenshot", SRender_SaveScreenShot);
	Core_0RegisterConcmd("save_worktex", SRender_SaveWorkTex);
	Core_0RegisterConcmd("shader_reload", SGCore_ShaderReloadAll);

#if defined(SX_GAME)
	Core_0RegisterConcmd("change_mode_window", SRender_ChangeModeWindow);
	Core_0RegisterConcmd("change_mode_window_abs", SRender_FullScreenChangeSizeAbs);
#endif

	Core_0ConsoleExecCmd("exec ../sysconfig.cfg");
	Core_0ConsoleExecCmd("exec ../userconfig.cfg");

	Core_0ConsoleUpdate();

	Core_0ConsoleExecCmd("exec ../sysconfig.cfg");
	Core_0ConsoleExecCmd("exec ../userconfig.cfg");
}

LRESULT CALLBACK SkyXEngine_WndProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	IMSG msg;
	msg.lParam = lParam;
	msg.wParam = wParam;
	msg.message = uiMessage;

	SSInput_AddMsg(msg);

	switch (uiMessage)
{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
		}

	return(DefWindowProc(hWnd, uiMessage, wParam, lParam));
	}

HWND SkyXEngine_CreateWindow(const char *szName, const char *szCaption, int iWidth, int iHeight)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = SkyXEngine_WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(0);
	wcex.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_LOGO));
	wcex.hCursor = 0;
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szName;
	wcex.hIconSm = wcex.hIcon;

	RegisterClassEx(&wcex);

	RECT rc = { 0, 0, iWidth, iHeight };
	AdjustWindowRect(&rc, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, false);

	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;

	int posx = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	int posy = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

	HWND hWnd = CreateWindowEx(
		0,
		szName,
		szCaption,
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		posx, posy,
		width, height,
		0, 0,
		GetModuleHandle(0),
		0);

	ShowWindow(hWnd, SW_HIDE);
	return hWnd;
}

//#############################################################################

void SkyXEngine_Frame(DWORD timeDelta)
{
	static IDirect3DDevice9 *pDXDevice = SGCore_GetDXDevice();
	static float3 vCamPos, vCamDir;
	static float4x4 mView, mProjLight;

	static const int *r_final_image = GET_PCVAR_INT("r_final_image");
	static const int *r_resize = GET_PCVAR_INT("r_resize");
	static bool isSimulationRender = false;

	static uint64_t DelayGeomSortGroup = 0;
	static uint64_t DelayComReflection = 0;
	static uint64_t DelayUpdateShadow = 0;
	static uint64_t DelayRenderMRT = 0;
	static uint64_t DelayComLighting = 0;
	static uint64_t DelayPostProcess = 0;
	static uint64_t DelayUpdateVisibleForCamera = 0;
	static uint64_t DelayUpdateVisibleForReflection = 0;
	static uint64_t DelayUpdateVisibleForLight = 0;
	static uint64_t DelayUpdateParticles = 0;

	static uint64_t DelayLibUpdateGame = 0;
	static uint64_t DelayLibUpdateLevel = 0;
	static uint64_t DelayLibUpdatePhysic = 0;
	static uint64_t DelayLibUpdateAnim = 0;

	static uint64_t DelayLibSyncGame = 0;
	static uint64_t DelayLibSyncPhysic = 0;
	static uint64_t DelayLibSyncAnim = 0;

	static uint64_t DelayPresent = 0;

#if defined(SX_MATERIAL_EDITOR)
	isSimulationRender = true;
#endif

	if (!pDXDevice)
	{
		SkyXEngine_PrintfLog(REPORT_MSG_LEVEL_ERROR, "dxdevice not found ...");
		return;
	}

	int64_t ttime;
	//потеряно ли устройство или произошло изменение размеров?
	if (pDXDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET || *r_resize)
	{
		//если не свернуто окно
		if (!IsIconic(SRender_GetHandleWin3D()) && ((SRender_GetParentHandleWin3D() != 0 && !IsIconic(SRender_GetParentHandleWin3D())) || SRender_GetParentHandleWin3D() == 0))
			SRender_ComDeviceLost();	//пытаемся восстановить
		return;
	}

#if !defined(SX_GAME) //&& !defined(SX_MATERIAL_EDITOR)
#if defined(SX_MATERIAL_EDITOR)
	if (SRender_EditorCameraGetMove())
#endif
		SRender_UpdateEditorial(timeDelta);
#endif

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SXAnim_Update();
	DelayLibUpdateAnim += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

#ifndef SX_PARTICLES_EDITOR

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SXGame_Update();
	DelayLibUpdateGame += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SLevel_WeatherUpdate();
	SLevel_AmbientSndUpdate();
	DelayLibUpdateLevel += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

#endif

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SXPhysics_Update();
	DelayLibUpdatePhysic += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SXAnim_Sync();
	DelayLibSyncAnim += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SXPhysics_Sync();
	DelayLibSyncPhysic += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

#ifndef SX_PARTICLES_EDITOR

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SXGame_Sync();
	DelayLibSyncGame += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;
#endif

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SGeom_ModelsMSortGroups(&vCamPos, 2);
	DelayGeomSortGroup += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;



	/**/
	/*if (SGeom_GreenGetOccurencessLeafGrass(&float3(vCamPos - float3(0.25, 1.8, 0.25)), &float3(vCamPos + float3(0.25, 0, 0.25)), MTLTYPE_PHYSIC_LEAF_GRASS))
		SXRenderFunc::Delay::FreeVal = 1;
	else
		SXRenderFunc::Delay::FreeVal = 0;*/
	/**/

	SRender_UpdateView();

	Core_RFloat3Get(G_RI_FLOAT3_OBSERVER_POSITION, &vCamPos);
	Core_RFloat3Get(G_RI_FLOAT3_OBSERVER_DIRECTION, &vCamDir);

	Core_RMatrixGet(G_RI_MATRIX_OBSERVER_VIEW, &mView);
	Core_RMatrixGet(G_RI_MATRIX_LIGHT_PROJ, &mProjLight);

	SML_Update(timeDelta);

	pDXDevice->BeginScene();
	pDXDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SRender_UpdateReflection(timeDelta, isSimulationRender);
	DelayComReflection += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	if (*r_final_image == DS_RT_AMBIENTDIFF || *r_final_image == DS_RT_SPECULAR || *r_final_image == DS_RT_SCENELIGHT)
	{
		//рендерим глубину от света
		ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
		SRender_UpdateShadow(timeDelta);
		DelayUpdateShadow += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;
	}

	//рисуем сцену и заполняем mrt данными
	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SRender_BuildMRT(timeDelta, isSimulationRender);
	DelayRenderMRT += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	if (*r_final_image == DS_RT_AMBIENTDIFF || *r_final_image == DS_RT_SPECULAR || *r_final_image == DS_RT_SCENELIGHT)
	{
		//освещаем сцену
		ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
		SRender_ComLighting(timeDelta);
		SRender_UnionLayers();
		if (SGCore_SkyBoxIsCr())
			SRender_RenderSky(timeDelta);
		SRender_ApplyToneMapping();
		SRender_ComToneMapping(timeDelta);

		DelayComLighting += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;
	}

	pDXDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	pDXDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3DZB_FALSE);

#if defined(SX_GAME)
	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SRender_RenderPostProcess(timeDelta);
	DelayPostProcess += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;
#endif

	SGCore_ShaderBindN(SHADER_TYPE_VERTEX, "pp_quad_render.vs");
	SGCore_ShaderBindN(SHADER_TYPE_PIXEL, "pp_quad_render.ps");

	pDXDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

#if !defined(SX_GAME)
	pDXDevice->SetTexture(0, SML_DSGetRT((DS_RT)*r_final_image));
#else
	pDXDevice->SetTexture(0, SGCore_RTGetTexture(SPP_RTGetCurrSend()));
#endif

	SGCore_ScreenQuadDraw();

	SGCore_ShaderUnBind();

	SRender_RenderParticles(timeDelta);

	pDXDevice->SetTransform(D3DTS_WORLD, &((D3DXMATRIX)SMMatrixIdentity()));
	pDXDevice->SetTransform(D3DTS_VIEW, &((D3DXMATRIX)mView));
	pDXDevice->SetTransform(D3DTS_PROJECTION, &((D3DXMATRIX)mProjLight));
	SRender_RenderEditorMain();


/*#if defined(_DEBUG)
	static const float * r_far = GET_PCVAR_FLOAT("r_far");
	SAIG_RenderQuads(SRender_GetCamera()->ObjFrustum, &vCamPos, *r_far);
#endif*/

#if defined(SX_GAME)
	SXGame_RenderHUD();
#endif

	
	

	
#if defined(SX_GAME) || defined(SX_LEVEL_EDITOR)
	static bool needGameTime = true;
#else
	static bool needGameTime = false;
#endif

	static char debugstr[4096];

	int FrameCount = 0;
	if ((FrameCount = SRender_OutputDebugInfo(timeDelta, needGameTime, debugstr)) > 0)
	{
		debugstr[0] = 0;
		
		sprintf(debugstr + strlen(debugstr), "\nCount poly: %d\n", Core_RIntGet(G_RI_INT_COUNT_POLY) / FrameCount);
		sprintf(debugstr + strlen(debugstr), "Count DIPs: %d\n", Core_RIntGet(G_RI_INT_COUNT_DIP) / FrameCount);

		sprintf(debugstr + strlen(debugstr), "\nPos camera: [ %.2f, %.2f, %.2f ]\n", vCamPos.x, vCamPos.y, vCamPos.z);
		sprintf(debugstr + strlen(debugstr), "Dir camera: [ %.2f, %.2f, %.2f ]\n", vCamDir.x, vCamDir.y, vCamDir.z);

		sprintf(debugstr + strlen(debugstr), "\nDelay:\n");
		sprintf(debugstr + strlen(debugstr), " Update shadow...: %.3f\n", float(DelayUpdateShadow) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), " Build G-buffer..: %.3f\n", float(DelayRenderMRT) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), " Lighting........: %.3f\n", float(DelayComLighting) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), " Postprocess.....: %.3f\n", float(DelayPostProcess) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), " Com reflection..: %.3f\n", float(DelayComReflection) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), " Geom sort group.: %.3f\n", float(DelayGeomSortGroup) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), " Update particles: %.3f\n", float(DelayUpdateParticles) / float(FrameCount) * 0.001f);

		sprintf(debugstr + strlen(debugstr), "\n UpdateVisibleFor\n");
		sprintf(debugstr + strlen(debugstr), "  Camera........: %.3f\n", float(DelayUpdateVisibleForCamera) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), "  Light.........: %.3f\n", float(DelayUpdateVisibleForLight) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), "  Reflection...: %.3f\n", float(DelayUpdateVisibleForReflection) / float(FrameCount) * 0.001f);

		sprintf(debugstr + strlen(debugstr), "\n LibUpdate. Game: %.3f, Level: %.3f, Physic: %.3f, Anim: %.3f\n", float(DelayLibUpdateGame) / float(FrameCount) * 0.001f, float(DelayLibUpdateLevel) / float(FrameCount) * 0.001f, float(DelayLibUpdatePhysic) / float(FrameCount) * 0.001f, float(DelayLibUpdateAnim) / float(FrameCount) * 0.001f);
		sprintf(debugstr + strlen(debugstr), " LibSync... Game: %.3f, Physic: %.3f, Anim: %.3f\n", float(DelayLibUpdateGame) / float(FrameCount) * 0.001f, float(DelayLibUpdatePhysic) / float(FrameCount) * 0.001f, float(DelayLibUpdateAnim) / float(FrameCount) * 0.001f);

		sprintf(debugstr + strlen(debugstr), "\n Present.........: %.3f\n", float(DelayPresent) / float(FrameCount) * 0.001f);

		sprintf(debugstr + strlen(debugstr), "\n### Engine version %s\n", SKYXENGINE_VERSION);

		Core_RIntSet(G_RI_INT_COUNT_POLY, 0);
		Core_RIntSet(G_RI_INT_COUNT_DIP, 0);

		DelayUpdateShadow = 0;
		DelayRenderMRT = 0;
		DelayComLighting = 0;
		DelayPostProcess = 0;
		DelayComReflection = 0;
		DelayGeomSortGroup = 0;
		DelayUpdateVisibleForCamera = 0;
		DelayUpdateVisibleForLight = 0;
		DelayUpdateVisibleForReflection = 0;
		DelayLibUpdateGame = DelayLibUpdateLevel = DelayLibUpdatePhysic = DelayLibUpdateAnim = 0;
		DelayLibSyncGame = DelayLibSyncPhysic = DelayLibSyncAnim = 0;
		DelayPresent = 0;
	}

#ifdef _DEBUG
	SXPhysics_DebugRender();
#endif

#if defined(SX_LEVEL_EDITOR)
	SXLevelEditor::LevelEditorUpdate(timeDelta);
#endif

#if defined(SX_MATERIAL_EDITOR)
	SXMaterialEditor::MaterialEditorUpdate(timeDelta);
#endif

#if defined(SX_PARTICLES_EDITOR)
	SXParticlesEditor::ParticlesEditorUpdate(timeDelta);
#endif

	SRender_ShaderRegisterData();

	pDXDevice->EndScene();

	//@@@
	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SRender_ComVisibleForCamera();
	DelayUpdateVisibleForCamera += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SRender_ComVisibleReflection();
	DelayUpdateVisibleForReflection += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SRender_ComVisibleForLight();
	DelayUpdateVisibleForLight += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	SPE_EffectVisibleComAll(SRender_GetCamera()->ObjFrustum, &vCamPos);
	SPE_EffectComputeAll();
	SPE_EffectComputeLightingAll();
	DelayUpdateParticles += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	ttime = TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	pDXDevice->Present(0, 0, 0, 0);
	DelayPresent += TimeGetMcsU(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - ttime;

	SkyXEngind_UpdateDataCVar();
}

void SkyXEngind_UpdateDataCVar()
{
	ID GlobalLight = SML_LigthsGetGlobal();
	static const bool * r_pssm_4or3 = GET_PCVAR_BOOL("r_pssm_4or3");
	static bool r_pssm_4or3_old = true;

	//проверяем не изменилось ли значение квара, если изменилось то меняем и количество сплитов
	if (r_pssm_4or3 && r_pssm_4or3_old != (*r_pssm_4or3) && GlobalLight >= 0)
	{
		r_pssm_4or3_old = (*r_pssm_4or3);
		SML_LigthsSet4Or3SplitsG(GlobalLight, r_pssm_4or3_old);
	}

	static const bool * r_pssm_shadowed = GET_PCVAR_BOOL("r_pssm_shadowed");
	static bool r_pssm_shadowed_old = true;

	if (r_pssm_shadowed && r_pssm_shadowed_old != (*r_pssm_shadowed) && GlobalLight >= 0)
	{
		r_pssm_shadowed_old = (*r_pssm_shadowed);
		SML_LigthsSetTypeShadowed(GlobalLight, (r_pssm_shadowed_old ? LTYPE_SHADOW_DYNAMIC : LTYPE_SHADOW_NONE));
	}

	static const float * r_pssm_quality = GET_PCVAR_FLOAT("r_pssm_quality");
	static float r_pssm_quality_old = 1;

	if (r_pssm_quality && r_pssm_quality_old != (*r_pssm_quality))
	{
		r_pssm_quality_old = (*r_pssm_quality);
		if (r_pssm_quality_old < 0.5f)
		{
			r_pssm_quality_old = 0.5f;
			Core_0SetCVarFloat("r_pssm_quality", r_pssm_quality_old);
		}
		else if (r_pssm_quality_old > 4.f)
		{
			r_pssm_quality_old = 4.f;
			Core_0SetCVarFloat("r_pssm_quality", r_pssm_quality_old);
		}
		SML_LigthsSettGCoefSizeDepth(r_pssm_quality_old);
	}

	static const float * r_lsm_quality = GET_PCVAR_FLOAT("r_lsm_quality");
	static float r_lsm_quality_old = 1;

	if (r_lsm_quality && r_lsm_quality_old != (*r_lsm_quality))
	{
		r_lsm_quality_old = (*r_lsm_quality);
		if (r_lsm_quality_old < 0.5f)
		{
			r_lsm_quality_old = 0.5f;
			Core_0SetCVarFloat("r_lsm_quality", r_lsm_quality_old);
		}
		else if (r_lsm_quality_old > 4.f)
		{
			r_lsm_quality_old = 4.f;
			Core_0SetCVarFloat("r_lsm_quality", r_lsm_quality_old);
		}
		SML_LigthsSettLCoefSizeDepth(r_lsm_quality_old);
	}

	static const int * r_grass_freq = GET_PCVAR_INT("r_grass_freq");
	static int r_grass_freq_old = 1;

	if (r_grass_freq && r_grass_freq_old != (*r_grass_freq))
	{
		r_grass_freq_old = (*r_grass_freq);
		if (r_grass_freq_old <= 0)
		{
			r_grass_freq_old = 1;
			Core_0SetCVarInt("r_grass_freq", r_grass_freq_old);
		}
		else if (r_grass_freq_old > 100)
		{
			r_grass_freq_old = 100;
			Core_0SetCVarInt("r_grass_freq", r_grass_freq_old);
		}
		SGeom_0SettGreenSetFreqGrass(r_grass_freq_old);
	}

	static const float * r_green_lod0 = GET_PCVAR_FLOAT("r_green_lod0");
	static float r_green_lod0_old = 50;

	if (r_green_lod0 && r_green_lod0_old != (*r_green_lod0))
	{
		r_green_lod0_old = (*r_green_lod0);
		if (r_green_lod0_old <= 20)
		{
			r_green_lod0_old = 20;
			Core_0SetCVarFloat("r_green_lod0", r_green_lod0_old);
		}
		else if (r_green_lod0_old > 100)
		{
			r_green_lod0_old = 100;
			Core_0SetCVarFloat("r_green_lod0", r_green_lod0_old);
		}
		SGeom_0SettGreenSetDistLods1(r_green_lod0_old);
	}

	static const float * r_green_lod1 = GET_PCVAR_FLOAT("r_green_lod1");
	static float r_green_lod1_old = 50;

	if (r_green_lod1 && r_green_lod1_old != (*r_green_lod1))
	{
		r_green_lod1_old = (*r_green_lod1);
		if (r_green_lod1_old <= 50)
		{
			r_green_lod1_old = 50;
			Core_0SetCVarFloat("r_green_lod1", r_green_lod1_old);
		}
		else if (r_green_lod1_old > 150)
		{
			r_green_lod1_old = 150;
			Core_0SetCVarFloat("r_green_lod1", r_green_lod1_old);
		}
		SGeom_0SettGreenSetDistLods2(r_green_lod1_old);
	}

	static const float * r_green_less = GET_PCVAR_FLOAT("r_green_less");
	static float r_green_less_old = 10;

	if (r_green_less && r_green_less_old != (*r_green_less))
	{
		r_green_less_old = (*r_green_less);
		if (r_green_less_old <= 10)
		{
			r_green_less_old = 10;
			Core_0SetCVarFloat("r_green_less", r_green_less_old);
		}
		else if (r_green_less_old > 90)
		{
			r_green_less_old = 90;
			Core_0SetCVarFloat("r_green_less", r_green_less_old);
		}
		SGeom_0SettGreenSetBeginEndLessening(r_green_less_old);
	}


	static int * r_win_width = (int*)GET_PCVAR_INT("r_win_width");
	static int * r_win_height = (int*)GET_PCVAR_INT("r_win_height");


	if (r_win_width && r_win_height)
	{

#ifdef SX_GAME
		static int r_win_width_old = *r_win_width;
		static int r_win_height_old = *r_win_height;

		static int iCountModes = 0;
		static const DEVMODE *aModes = SGCore_GetModes(&iCountModes);

		if (r_win_width && r_win_width_old != (*r_win_width) && r_win_height && r_win_height_old != (*r_win_height))
		{
			bool isValid = false;
			for (int i = 0; i < iCountModes; ++i)
			{
				if (aModes[i].dmPelsWidth == (*r_win_width) && aModes[i].dmPelsHeight == (*r_win_height))
				{
					isValid = true;
					break;
				}
			}

			if (isValid)
			{
				r_win_width_old = (*r_win_width);
				r_win_height_old = (*r_win_height);


				RECT rc = { 0, 0, r_win_width_old, r_win_height_old };
				AdjustWindowRect(&rc, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, false);

				int iWidth = rc.right - rc.left;
				int iHeight = rc.bottom - rc.top;

				GetWindowRect(SRender_GetHandleWin3D(), &rc);

				MoveWindow(SRender_GetHandleWin3D(), rc.left, rc.top, iWidth, iHeight, TRUE);

				static int *r_resize = (int*)GET_PCVAR_INT("r_resize");
				*r_resize = RENDER_RESIZE_RESIZE;
			}
			else
			{
				*r_win_width = r_win_width_old;
				*r_win_height = r_win_height_old;
			}
		}

		static const bool *r_win_windowed = GET_PCVAR_BOOL("r_win_windowed");

		if (r_win_windowed)
		{
			static bool r_win_windowed_old = *r_win_windowed;

			if (r_win_windowed_old != (*r_win_windowed))
			{
				r_win_windowed_old = (*r_win_windowed);
				static int *r_resize = (int*)GET_PCVAR_INT("r_resize");
				*r_resize = RENDER_RESIZE_CHANGE;
			}
		}
#else
		static bool *r_win_windowed = (bool*)GET_PCVAR_BOOL("r_win_windowed");
		*r_win_windowed = true;
#endif
	}
}

//#############################################################################

int SkyXEngine_CycleMain()
{
	MSG msg;
	::ZeroMemory(&msg, sizeof(MSG));

	while (msg.message != WM_QUIT && IsWindow(SRender_GetHandleWin3D()))
	{
		if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);

#if !defined(SX_GAME)
			IMSG imsg;
			imsg.lParam = msg.lParam;
			imsg.wParam = msg.wParam;
			imsg.message = msg.message;

			SSInput_AddMsg(imsg);
#endif

			::DispatchMessage(&msg);
		}
		else
		{
			SGCore_LoadTexAllLoad();
			SGCore_ShaderAllLoad();
			Core_TimesUpdate();
			Core_0ConsoleUpdate();
			SSInput_Update();
			static float3 vCamPos, vCamDir;
			Core_RFloat3Get(G_RI_FLOAT3_OBSERVER_POSITION, &vCamPos);
			Core_RFloat3Get(G_RI_FLOAT3_OBSERVER_DIRECTION, &vCamDir);
			SSCore_Update(&vCamPos, &vCamDir);

			static DWORD lastTime = TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER));
			DWORD currTime = TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER));
			DWORD timeDelta = (currTime - lastTime);
			Core_RIntSet(G_RI_INT_TIME_DELTA, timeDelta);
#ifdef SX_GAME
			SRender_SetCamera(SXGame_GetActiveCamera());
#endif


			static const bool * g_time_run = GET_PCVAR_BOOL("g_time_run");
			static bool g_time_run_old = true;

			if (g_time_run && g_time_run_old != (*g_time_run))
			{
				g_time_run_old = (*g_time_run);
				Core_TimeWorkingSet(Core_RIntGet(G_RI_INT_TIMER_GAME), g_time_run_old);
			}

			static const float * g_time_speed = GET_PCVAR_FLOAT("g_time_speed");
			static float g_time_speed_old = true;

			if (g_time_speed && g_time_speed_old != (*g_time_speed))
			{
				g_time_speed_old = (*g_time_speed);
				Core_TimeSpeedSet(Core_RIntGet(G_RI_INT_TIMER_GAME), g_time_speed_old);
			}


			if (Core_TimeWorkingGet(Core_RIntGet(G_RI_INT_TIMER_RENDER)) && (GetForegroundWindow() == SRender_GetHandleWin3D() || GetForegroundWindow() == (HWND)SRender_GetParentHandleWin3D() || GetForegroundWindow() == FindWindow(NULL, "sxconsole")))
			{

#if defined(SX_LEVEL_EDITOR)
				SXLevelEditor_Transform(10);
#endif

				SkyXEngine_Frame(timeDelta);
			}

			lastTime = currTime;
		}
	}

	return msg.wParam;
}

//#############################################################################

void SkyXEngine_Kill()
{
/*#if defined(SX_MATERIAL_EDITOR)
	mem_delete(GData::Editors::SimModel);
#endif

#if !defined(SX_GAME)
	mem_delete(GData::Editors::ObjGrid);
	mem_delete(GData::Editors::ObjAxesStatic);
	mem_release(GData::Editors::FigureBox);
	mem_release(GData::Editors::FigureSphere);
	mem_release(GData::Editors::FigureCone);
	mem_delete(GData::Editors::ObjAxesHelper);
#endif
*/
#if !defined(SX_PARTICLES_EDITOR)
	SXGame_AKill();
#endif
	SPE_AKill();
	SXDecals_AKill();
	SXPhysics_AKill();
	SXAnim_AKill();

	SGeom_AKill();
	SML_AKill();
	SSCore_AKill();
	SGCore_AKill();
	Core_AKill();
}

//#############################################################################

LRESULT CALLBACK SkyXEngine_PreviewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HDC hmdc;
	HBITMAP hBitmap;
	BITMAP bm;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		hmdc = CreateCompatibleDC(hdc);
		hBitmap = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP_PREVIEW));
		SelectObject(hmdc, hBitmap);
		GetObject(hBitmap, sizeof(bm), &bm);
		BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hmdc, 0, 0, SRCCOPY);
		DeleteDC(hmdc);
		DeleteObject(hBitmap);
		EndPaint(hWnd, &ps);
		break;
	}

	return(DefWindowProc(hWnd, message, wParam, lParam));
}

HWND g_hWinPreview = 0;

void SkyXEngine_PreviewCreate()
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = SkyXEngine_PreviewWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(0);
	wcex.hIcon = NULL;
	wcex.hCursor = 0;
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "SkyXEngine_Preview";
	wcex.hIconSm = 0;

	RegisterClassEx(&wcex);

	int width = 1024;
	int height = 256;

	int posx = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	int posy = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

	g_hWinPreview = CreateWindowEx(
		/*WS_EX_TOPMOST*/0,
		"SkyXEngine_Preview",
		"SkyXEngine_Preview",
		WS_POPUP,
		(GetSystemMetrics(SM_CXSCREEN) - width) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - height) / 2,
		width, height,
		0, 0,
		GetModuleHandle(0),
		0);

	ShowWindow(g_hWinPreview, SW_SHOW);

	DWORD ttime = GetTickCount();
	MSG msg;
	::ZeroMemory(&msg, sizeof(MSG));

	while (true)
	{
		if (GetTickCount() - ttime > 500)
			break;

		if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
}

void SkyXEngine_PreviewKill()
{
	DestroyWindow(g_hWinPreview);
	g_hWinPreview = 0;
}

//##########################################################################

void SkyXEngine_RFuncDIP(UINT type_primitive, long base_vertexIndex, UINT min_vertex_index, UINT num_vertices, UINT start_index, UINT prim_count)
{

}

void SkyXEngine_RFuncMtlSet(ID id, float4x4* world)
{
	switch (Core_RIntGet(G_RI_INT_RENDERSTATE))
	{
	case RENDER_STATE_SHADOW:
		SML_MtlSetMainTexture(0, id);
		SML_LigthsShadowSetShaderOfTypeMat(Core_RIntGet(G_RI_INT_CURRIDLIGHT), SML_MtlGetTypeModel(id), world);
		break;

	case RENDER_STATE_FREE:
		SML_MtlSetMainTexture(0, id);
		Core_RMatrixSet(G_RI_MATRIX_WORLD, &(world ? (*world) : SMMatrixIdentity()));
		//SGCore_ShaderUnBind();
		SML_MtlRenderStd(SML_MtlGetTypeModel(id), world, 0, id);
		break;

	case RENDER_STATE_MATERIAL:
		SML_MtlRender(id, world);
		break;
	}
}

ID SkyXEngine_RFuncMtlLoad(const char* name, int mtl_type)
{
	return SML_MtlLoad(name, (MTLTYPE_MODEL)mtl_type);
}

bool SkyXEngine_RFuncAIQuadPhyNavigate(float3_t * pos)
{
	static btBoxShape boxfull(btVector3(AIGRID_QUAD_SIZEDIV2, AIGRID_ENTITY_MAX_HEIGHTDIV2, AIGRID_QUAD_SIZEDIV2));
	float3 start = (*pos);
	start.y = pos->y + AIGRID_ENTITY_MAX_HEIGHT;
	float3 end = (*pos);
	//end.y = min->y - AIGRID_ENTITY_MAX_HEIGHT;
	static btDiscreteDynamicsWorld::ClosestConvexResultCallback cb(F3_BTVEC(start), F3_BTVEC(end));
	cb = btDiscreteDynamicsWorld::ClosestConvexResultCallback(F3_BTVEC(start), F3_BTVEC(end));

	static btTransform xForm;
	xForm.setOrigin(F3_BTVEC(start));
	xForm.getBasis().setIdentity();
	static btTransform xForm2;
	xForm2.setOrigin(F3_BTVEC(end));
	xForm2.getBasis().setIdentity();
	SXPhysics_GetDynWorld()->convexSweepTest(&boxfull, xForm, xForm2, cb);

	if (cb.hasHit())
	{
		pos->y = cb.m_hitPointWorld[1];
		//quad->IsClose = false;

		static btBoxShape boxoff(btVector3(AIGRID_QUAD_SIZEDIV2, (AIGRID_ENTITY_MAX_HEIGHT - AIGRID_QUADS_CENTERS_MAXHEIGHT) * 0.5, AIGRID_QUAD_SIZEDIV2));

		start = (*pos);
		start.y = pos->y + AIGRID_ENTITY_MAX_HEIGHTDIV2 + AIGRID_QUADS_CENTERS_MAXHEIGHT;
		static btVector3 vec;
		vec = btVector3(F3_BTVEC(start));
		cb = btDiscreteDynamicsWorld::ClosestConvexResultCallback(vec, vec);
		static btVector3 offs;
		for (int x = -1; x <= 1; ++x)
		{
			for (int z = -1; z <= 1; ++z)
			{
				offs[0] = 0.5f*float(x) * 0.01f;
				offs[1] = 1.f * 0.01f;
				offs[2] = 0.5f*float(z) * 0.01f;
				xForm.setOrigin(vec - offs);
				xForm.getBasis().setIdentity();
				xForm2.setOrigin(vec + offs);
				xForm2.getBasis().setIdentity();
				SXPhysics_GetDynWorld()->convexSweepTest(&boxoff, xForm, xForm2, cb);

				if (cb.hasHit())
				{
					return true;
				}
			}
		}
	}
	else
		return true;

	return false;
}

bool SkyXEngine_RFuncParticlesPhyCollision(const float3 * lastpos, const float3* nextpos, float3* coll_pos, float3* coll_nrm)
{
	if (!lastpos || !nextpos)
		return false;

	if (lastpos->x == nextpos->x && lastpos->y == nextpos->y && lastpos->z == nextpos->z)
		return false;

	btCollisionWorld::ClosestRayResultCallback cb(F3_BTVEC(*lastpos), F3_BTVEC(*nextpos));
	SXPhysics_GetDynWorld()->rayTest(F3_BTVEC(*lastpos), F3_BTVEC(*nextpos), cb);

	if (cb.hasHit())
	{
		if (coll_pos)
			*coll_pos = BTVEC_F3(cb.m_hitPointWorld);

		if (coll_nrm)
			*coll_nrm = BTVEC_F3(cb.m_hitNormalWorld);

		return true;
	}

	return false;
}