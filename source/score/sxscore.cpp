
/******************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017
See the license in LICENSE
******************************************************/

#define SXSCORE_VERSION 1

#if !defined(DEF_STD_REPORT)
#define DEF_STD_REPORT
report_func reportf = def_report;
#endif

#include <score\\sxscore.h>
#include <score\\sound.cpp>

MainSound* MSound = 0;

#define SCORE_PRECOND(retval) if(!MSound){reportf(-1, "%s - sxsound is not init", gen_msg_location); return retval;}

long SSCore_0GetVersion()
{
	return SXSCORE_VERSION;
}

void SSCore_Dbg_Set(report_func rf)
{
	reportf = rf;
}

void SSCore_0Create(const char* name, HWND hwnd, const char* std_path_sound, bool is_unic)
{
	if (name && strlen(name) > 1)
	{
		if (is_unic)
		{
			HANDLE hMutex = CreateMutex(NULL, FALSE, name);
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(hMutex);
				reportf(-1, "%s - none unic name, sxsound", gen_msg_location);
			}
			else
			{
				MSound = new MainSound();
				MSound->Init(hwnd);
				MSound->StdPathSet(std_path_sound);
			}
		}
		else
		{
			MSound = new MainSound();
			MSound->Init(hwnd);
			MSound->StdPathSet(std_path_sound);
		}
	}
	else
		reportf(-1, "%s - not init argument [name], sxsound", gen_msg_location);
}

void SSCore_AKill()
{
	mem_delete(MSound);
}

//#############################################################################

void SSCore_StdPathSet(const char* path)
{
	SCORE_PRECOND(_VOID);

	MSound->StdPathSet(path);
}

void SSCore_StdPathGet(char* path)
{
	SCORE_PRECOND(_VOID);

	MSound->StdPathGet(path);
}

void SSCore_Clear()
{
	SCORE_PRECOND(_VOID);

	MSound->Clear();
}


void SSCore_Update(float3* viewpos, float3* viewdir)
{
	SCORE_PRECOND(_VOID);

	MSound->Update(viewpos, viewdir);
}

int SSCore_SndsPlayCountGet()
{
	SCORE_PRECOND(0);

	return MSound->SoundsPlayCountGet();
}

int SSCore_SndsLoadCountGet()
{
	SCORE_PRECOND(0);

	return MSound->SoundsLoadCountGet();
}

ID SSCore_SndCreate2d(const char *file, bool looping, DWORD size_stream)
{
	SCORE_PRECOND(-1);

	return MSound->SoundCreate2d(file, looping, size_stream);
}

ID SSCore_SndCreate3d(const char *file, bool looping, DWORD size_stream, float dist, float shift_pan)
{
	SCORE_PRECOND(-1);

	return MSound->SoundCreate3d(file, looping, size_stream, dist, shift_pan);
}

ID SSCore_SndCreate2dInst(const char *file, bool looping, DWORD size_stream)
{
	SCORE_PRECOND(-1);

	return  MSound->SoundCreate2dInst(file, looping, size_stream);
}

ID SSCore_SndCreate3dInst(const char *file, bool looping, DWORD size_stream, float dist, float shift_pan)
{
	SCORE_PRECOND(-1);

	return  MSound->SoundCreate3dInst(file, looping, size_stream, dist, shift_pan);
}

ID SSCore_SndFind2dInst(const char * file)
{
	SCORE_PRECOND(-1);

	return MSound->SoundFind2dInst(file);
}

ID SSCore_SndFind3dInst(const char * file)
{
	SCORE_PRECOND(-1);

	return MSound->SoundFind3dInst(file);
}

void SSCore_SndInstancePlay2d(ID id, int volume, int pan)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundInstancePlay2d(id, volume, pan);
}

void SSCore_SndInstancePlay3d(ID id, float3* pos)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundInstancePlay3d(id, pos);
}


bool SSCore_SndIsInit(ID id)
{
	SCORE_PRECOND(false);

	return MSound->SoundIsInit(id);
}

void SSCore_SndDelete(ID id)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundDelete(id);
}


void SSCore_SndPlay(ID id, int looping)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundPlay(id, looping);
}

void SSCore_SndPause(ID id)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundPause(id);
}

void SSCore_SndStop(ID id)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundStop(id);
}


void SSCore_SndStateSet(ID id, SoundObjState state)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundStateSet(id, state);
}

SoundObjState SSCore_SndStateGet(ID id)
{
	SCORE_PRECOND(SoundObjState::sos_stop);

	return MSound->SoundStateGet(id);
}

void SSCore_SndPosCurrSet(ID id, DWORD pos, int type)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundPosCurrSet(id, pos, type);
}

DWORD SSCore_SndPosCurrGet(ID id, int type)
{
	SCORE_PRECOND(0);

	return MSound->SoundPosCurrGet(id, type);
}


void SSCore_SndVolumeSet(ID id, long volume, int type)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundVolumeSet(id, volume, type);
}

long SSCore_SndVolumeGet(ID id, int type)
{
	SCORE_PRECOND(0);

	return MSound->SoundVolumeGet(id, type);
}


void SSCore_SndPanSet(ID id, long value, int type)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundPanSet(id, value, type);
}

long SSCore_SndPanGet(ID id, int type)
{
	SCORE_PRECOND(0);

	return MSound->SoundPanGet(id, type);
}


void SSCore_SndFreqCurrSet(ID id, DWORD value)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundFreqCurrSet(id, value);
}

DWORD SSCore_SndFreqCurrGet(ID id)
{
	SCORE_PRECOND(0);

	return MSound->SoundFreqCurrGet(id);
}

DWORD SSCore_SndFreqOriginGet(ID id)
{
	SCORE_PRECOND(0);

	return MSound->SoundFreqOriginGet(id);
}

void SSCore_SndPosWSet(ID id, float3* pos)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundPosWSet(id, pos);
}

void SSCore_SndPosWGet(ID id, float3* pos)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundPosWGet(id, pos);
}


int SSCore_SndLengthSecGet(ID id)
{
	SCORE_PRECOND(0);

	return MSound->SoundLengthSecGet(id);
}

DWORD SSCore_SndBytesPerSecGet(ID id)
{
	SCORE_PRECOND(0);

	return MSound->SoundBytesPerSecGet(id);
}

DWORD SSCore_SndSizeGet(ID id)
{
	SCORE_PRECOND(0);

	return MSound->SoundSizeGet(id);
}

void SSCore_SndFileGet(ID id, char* path)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundFileGet(id, path);
}


float SSCore_SndDistAudibleGet(ID id)
{
	SCORE_PRECOND(0);

	return MSound->SoundDistAudibleGet(id);
}

void SSCore_SndDistAudibleSet(ID id, float value)
{
	SCORE_PRECOND(_VOID);

	MSound->SoundDistAudibleSet(id, value);
}
