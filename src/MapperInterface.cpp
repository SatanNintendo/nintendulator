/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#ifdef	NSFPLAYER
# include "in_nintendulator.h"
# include "MapperInterface.h"
# include "CPU.h"
#else	/* !NSFPLAYER */
# include "stdafx.h"
# include "Nintendulator.h"
# include "resource.h"
# include "MapperInterface.h"
# include "NES.h"
# include "CPU.h"
# include "PPU.h"
# include "GFX.h"
# include "Debugger.h"
#endif	/* NSFPLAYER */

EmulatorInterface EI;
ROMInfo RI;

DLLInfo *DI;
const MapperInfo *MI;
#ifndef	NSFPLAYER
const MapperInfo *MI2;
#endif	/* !NSFPLAYER */

#ifdef	NSFPLAYER
HINSTANCE		dInst;
FLoadMapperDLL		LoadDLL;
FUnloadMapperDLL	UnloadDLL;
#else	/* !NSFPLAYER */
struct	MapperDLL
{
	HINSTANCE	dInst;
	FLoadMapperDLL	LoadDLL;
	DLLInfo		*DI;
	FUnloadMapperDLL	UnloadDLL;
	MapperDLL	*Next;
} *MapperDLLs = NULL;
#endif	/* NSFPLAYER */

/******************************************************************************/

static	void	MAPINT	SetCPUWriteHandler (int Page, FCPUWrite New)
{
	CPU::WriteHandler[Page] = New;
}
static	void	MAPINT	SetCPUReadHandler (int Page, FCPURead New)
{
	CPU::ReadHandler[Page] = New;
}
static	FCPUWrite	MAPINT	GetCPUWriteHandler (int Page)
{
	return CPU::WriteHandler[Page];
}
static	FCPURead	MAPINT	GetCPUReadHandler (int Page)
{
	return CPU::ReadHandler[Page];
}

static	void	MAPINT	SetPPUWriteHandler (int Page, FPPUWrite New)
{
#ifndef	NSFPLAYER
	PPU::WriteHandler[Page] = New;
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	SetPPUReadHandler (int Page, FPPURead New)
{
#ifndef	NSFPLAYER
	PPU::ReadHandler[Page] = New;
#endif	/* !NSFPLAYER */
}
static	FPPUWrite	MAPINT	GetPPUWriteHandler (int Page)
{
#ifndef	NSFPLAYER
	return PPU::WriteHandler[Page];
#else	/* NSFPLAYER */
	return NULL;
#endif	/* !NSFPLAYER */
}
static	FPPURead	MAPINT	GetPPUReadHandler (int Page)
{
#ifndef	NSFPLAYER
	return PPU::ReadHandler[Page];
#else	/* NSFPLAYER */
	return NULL;
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	__inline void	MAPINT	SetPRG_ROM4 (int Bank, int Val)
{
	CPU::PRGPointer[Bank] = NES::PRG_ROM[Val & NES::PRGMask];
	CPU::Readable[Bank] = TRUE;
	CPU::Writable[Bank] = FALSE;
}
static	void	MAPINT	SetPRG_ROM8 (int Bank, int Val)
{
	Val <<= 1;
	SetPRG_ROM4(Bank+0, Val+0);
	SetPRG_ROM4(Bank+1, Val+1);
}
static	void	MAPINT	SetPRG_ROM16 (int Bank, int Val)
{
	Val <<= 2;
	SetPRG_ROM4(Bank+0, Val+0);
	SetPRG_ROM4(Bank+1, Val+1);
	SetPRG_ROM4(Bank+2, Val+2);
	SetPRG_ROM4(Bank+3, Val+3);
}
static	void	MAPINT	SetPRG_ROM32 (int Bank, int Val)
{
	Val <<= 3;
	SetPRG_ROM4(Bank+0, Val+0);
	SetPRG_ROM4(Bank+1, Val+1);
	SetPRG_ROM4(Bank+2, Val+2);
	SetPRG_ROM4(Bank+3, Val+3);
	SetPRG_ROM4(Bank+4, Val+4);
	SetPRG_ROM4(Bank+5, Val+5);
	SetPRG_ROM4(Bank+6, Val+6);
	SetPRG_ROM4(Bank+7, Val+7);
}
static	int	MAPINT	GetPRG_ROM4 (int Bank)	// -1 if no ROM mapped
{
	int tpi = (int)((CPU::PRGPointer[Bank] - NES::PRG_ROM[0]) >> 12);
	if ((tpi < 0) || (tpi > NES::PRGMask)) return -1;
	else return tpi;
}

/******************************************************************************/

static	__inline void	MAPINT	SetPRG_RAM4 (int Bank, int Val)
{
	CPU::PRGPointer[Bank] = NES::PRG_RAM[Val & 0xF];
	CPU::Readable[Bank] = TRUE;
	CPU::Writable[Bank] = TRUE;
}
static	void	MAPINT	SetPRG_RAM8 (int Bank, int Val)
{
	Val <<= 1;
	SetPRG_RAM4(Bank+0, Val+0);
	SetPRG_RAM4(Bank+1, Val+1);
}
static	void	MAPINT	SetPRG_RAM16 (int Bank, int Val)
{
	Val <<= 2;
	SetPRG_RAM4(Bank+0, Val+0);
	SetPRG_RAM4(Bank+1, Val+1);
	SetPRG_RAM4(Bank+2, Val+2);
	SetPRG_RAM4(Bank+3, Val+3);
}
static	void	MAPINT	SetPRG_RAM32 (int Bank, int Val)
{
	Val <<= 3;
	SetPRG_RAM4(Bank+0, Val+0);
	SetPRG_RAM4(Bank+1, Val+1);
	SetPRG_RAM4(Bank+2, Val+2);
	SetPRG_RAM4(Bank+3, Val+3);
	SetPRG_RAM4(Bank+4, Val+4);
	SetPRG_RAM4(Bank+5, Val+5);
	SetPRG_RAM4(Bank+6, Val+6);
	SetPRG_RAM4(Bank+7, Val+7);
}
static	int	MAPINT	GetPRG_RAM4 (int Bank)	// -1 if no RAM mapped
{
	int tpi = (int)((CPU::PRGPointer[Bank] - NES::PRG_RAM[0]) >> 12);
	if ((tpi < 0) || (tpi > MAX_PRGRAM_MASK)) return -1;
	else return tpi;
}

/******************************************************************************/

static	unsigned char *	MAPINT	GetPRG_Ptr4 (int Bank)
{
	return CPU::PRGPointer[Bank];
}
static	void	MAPINT	SetPRG_Ptr4 (int Bank, unsigned char *Data, BOOL Writable)
{
	CPU::PRGPointer[Bank] = Data;
	CPU::Readable[Bank] = TRUE;
	CPU::Writable[Bank] = Writable;
}
static	void	MAPINT	SetPRG_OB4 (int Bank)	// Open bus
{
	CPU::Readable[Bank] = FALSE;
	CPU::Writable[Bank] = FALSE;
}

/******************************************************************************/

static	__inline void	MAPINT	SetCHR_ROM1 (int Bank, int Val)
{
#ifndef	NSFPLAYER
	if (!NES::CHRMask)
		return;
	PPU::CHRPointer[Bank] = NES::CHR_ROM[Val & NES::CHRMask];
	PPU::Writable[Bank] = FALSE;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	SetCHR_ROM2 (int Bank, int Val)
{
	Val <<= 1;
	SetCHR_ROM1(Bank+0, Val+0);
	SetCHR_ROM1(Bank+1, Val+1);
}
static	void	MAPINT	SetCHR_ROM4 (int Bank, int Val)
{
	Val <<= 2;
	SetCHR_ROM1(Bank+0, Val+0);
	SetCHR_ROM1(Bank+1, Val+1);
	SetCHR_ROM1(Bank+2, Val+2);
	SetCHR_ROM1(Bank+3, Val+3);
}
static	void	MAPINT	SetCHR_ROM8 (int Bank, int Val)
{
	Val <<= 3;
	SetCHR_ROM1(Bank+0, Val+0);
	SetCHR_ROM1(Bank+1, Val+1);
	SetCHR_ROM1(Bank+2, Val+2);
	SetCHR_ROM1(Bank+3, Val+3);
	SetCHR_ROM1(Bank+4, Val+4);
	SetCHR_ROM1(Bank+5, Val+5);
	SetCHR_ROM1(Bank+6, Val+6);
	SetCHR_ROM1(Bank+7, Val+7);
}
static	int	MAPINT	GetCHR_ROM1 (int Bank)	// -1 if no ROM mapped
{
#ifndef	NSFPLAYER
	int tpi = (int)((PPU::CHRPointer[Bank] - NES::CHR_ROM[0]) >> 10);
	if ((tpi < 0) || (tpi > NES::CHRMask)) return -1;
	else return tpi;
#else	/* NSFPLAYER */
	return -1;
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	__inline void	MAPINT	SetCHR_RAM1 (int Bank, int Val)
{
#ifndef	NSFPLAYER
	PPU::CHRPointer[Bank] = NES::CHR_RAM[Val & 0x1F];
	PPU::Writable[Bank] = TRUE;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	SetCHR_RAM2 (int Bank, int Val)
{
	Val <<= 1;
	SetCHR_RAM1(Bank+0, Val+0);
	SetCHR_RAM1(Bank+1, Val+1);
}
static	void	MAPINT	SetCHR_RAM4 (int Bank, int Val)
{
	Val <<= 2;
	SetCHR_RAM1(Bank+0, Val+0);
	SetCHR_RAM1(Bank+1, Val+1);
	SetCHR_RAM1(Bank+2, Val+2);
	SetCHR_RAM1(Bank+3, Val+3);
}
static	void	MAPINT	SetCHR_RAM8 (int Bank, int Val)
{
	Val <<= 3;
	SetCHR_RAM1(Bank+0, Val+0);
	SetCHR_RAM1(Bank+1, Val+1);
	SetCHR_RAM1(Bank+2, Val+2);
	SetCHR_RAM1(Bank+3, Val+3);
	SetCHR_RAM1(Bank+4, Val+4);
	SetCHR_RAM1(Bank+5, Val+5);
	SetCHR_RAM1(Bank+6, Val+6);
	SetCHR_RAM1(Bank+7, Val+7);
}
static	int	MAPINT	GetCHR_RAM1 (int Bank)	// -1 if no ROM mapped
{
#ifndef	NSFPLAYER
	int tpi = (int)((PPU::CHRPointer[Bank] - NES::CHR_RAM[0]) >> 10);
	if ((tpi < 0) || (tpi > MAX_CHRRAM_MASK)) return -1;
	else return tpi;
#else	/* NSFPLAYER */
	return -1;
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	__inline void	MAPINT	SetCHR_NT1 (int Bank, int Val)
{
#ifndef	NSFPLAYER
	PPU::CHRPointer[Bank] = PPU::VRAM[Val & 3];
	PPU::Writable[Bank] = TRUE;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}
static	int	MAPINT	GetCHR_NT1 (int Bank)	// -1 if no ROM mapped
{
#ifndef	NSFPLAYER
	int tpi = (int)((PPU::CHRPointer[Bank] - PPU::VRAM[0]) >> 10);
	if ((tpi < 0) || (tpi > 4)) return -1;
	else return tpi;
#else	/* NSFPLAYER */
	return -1;
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	unsigned char *	MAPINT	GetCHR_Ptr1 (int Bank)
{
#ifndef	NSFPLAYER
	return PPU::CHRPointer[Bank];
#else	/* NSFPLAYER */
	return NULL;
#endif	/* !NSFPLAYER */
}

static	void	MAPINT	SetCHR_Ptr1 (int Bank, unsigned char *Data, BOOL Writable)
{
#ifndef	NSFPLAYER
	PPU::CHRPointer[Bank] = Data;
	PPU::Writable[Bank] = Writable;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}

static	void	MAPINT	SetCHR_OB1 (int Bank)
{
#ifndef	NSFPLAYER
	PPU::CHRPointer[Bank] = PPU::OpenBus;
	PPU::Writable[Bank] = FALSE;
#ifdef	ENABLE_DEBUGGER
	if (Bank & 8)
		Debugger::NTabChanged = TRUE;
	else	Debugger::PatChanged = TRUE;
#endif	/* ENABLE_DEBUGGER */
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	void	MAPINT	Mirror_H (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8, 0);	SetCHR_NT1(0xC, 0);
	SetCHR_NT1(0x9, 0);	SetCHR_NT1(0xD, 0);
	SetCHR_NT1(0xA, 1);	SetCHR_NT1(0xE, 1);
	SetCHR_NT1(0xB, 1);	SetCHR_NT1(0xF, 1);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_V (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8, 0);	SetCHR_NT1(0xC, 0);
	SetCHR_NT1(0x9, 1);	SetCHR_NT1(0xD, 1);
	SetCHR_NT1(0xA, 0);	SetCHR_NT1(0xE, 0);
	SetCHR_NT1(0xB, 1);	SetCHR_NT1(0xF, 1);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_4 (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8, 0);	SetCHR_NT1(0xC, 0);
	SetCHR_NT1(0x9, 1);	SetCHR_NT1(0xD, 1);
	SetCHR_NT1(0xA, 2);	SetCHR_NT1(0xE, 2);
	SetCHR_NT1(0xB, 3);	SetCHR_NT1(0xF, 3);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_S0 (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8, 0);	SetCHR_NT1(0xC, 0);
	SetCHR_NT1(0x9, 0);	SetCHR_NT1(0xD, 0);
	SetCHR_NT1(0xA, 0);	SetCHR_NT1(0xE, 0);
	SetCHR_NT1(0xB, 0);	SetCHR_NT1(0xF, 0);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_S1 (void)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8, 1);	SetCHR_NT1(0xC, 1);
	SetCHR_NT1(0x9, 1);	SetCHR_NT1(0xD, 1);
	SetCHR_NT1(0xA, 1);	SetCHR_NT1(0xE, 1);
	SetCHR_NT1(0xB, 1);	SetCHR_NT1(0xF, 1);
#endif	/* !NSFPLAYER */
}
static	void	MAPINT	Mirror_Custom (int M1, int M2, int M3, int M4)
{
#ifndef	NSFPLAYER
	SetCHR_NT1(0x8, M1);	SetCHR_NT1(0xC, M1);
	SetCHR_NT1(0x9, M2);	SetCHR_NT1(0xD, M2);
	SetCHR_NT1(0xA, M3);	SetCHR_NT1(0xE, M3);
	SetCHR_NT1(0xB, M4);	SetCHR_NT1(0xF, M4);
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	void	MAPINT	Set_SRAMSize (int Size)	// Sets the size of the SRAM (in bytes) and clears PRG_RAM
{
#ifndef	NSFPLAYER
	NES::SRAM_Size = Size;
	ZeroMemory(NES::PRG_RAM, sizeof(NES::PRG_RAM));
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

static	void	MAPINT	SetIRQ (int IRQstate)
{
	if (IRQstate)
		CPU::WantIRQ &= ~IRQ_EXTERNAL;
	else	CPU::WantIRQ |= IRQ_EXTERNAL;
}

static	void	MAPINT	DbgOut (TCHAR *text, ...)
{
#ifndef	NSFPLAYER
	static TCHAR txt[1024];
	va_list marker;
	va_start(marker, text);
	_vstprintf(txt, text, marker);
	va_end(marker);
	AddDebug(txt);
#endif	/* !NSFPLAYER */
}

static	void	MAPINT	StatusOut (TCHAR *text, ...)
{
#ifndef	NSFPLAYER
	static TCHAR txt[1024];
	va_list marker;
	va_start(marker, text);
	_vstprintf(txt, text, marker);
	va_end(marker);
	PrintTitlebar(txt);
#endif	/* !NSFPLAYER */
}

/******************************************************************************/

namespace MapperInterface
{
void	Init (void)
{
#ifndef	NSFPLAYER
	WIN32_FIND_DATA Data;
	HANDLE Handle;
	MapperDLL *ThisDLL;
	TCHAR Filename[MAX_PATH], Path[MAX_PATH];
	_tcscpy(Path, ProgPath);
	_tcscat(Path, _T("Mappers\\"));
	_stprintf(Filename, _T("%s%s"), Path, _T("*.dll"));
	Handle = FindFirstFile(Filename, &Data);
	ThisDLL = new MapperDLL;
	if (Handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR Tmp[MAX_PATH];
			_stprintf(Tmp, _T("%s%s"), Path, Data.cFileName);
			ThisDLL->dInst = LoadLibrary(Tmp);
			if (!ThisDLL->dInst)
			{
				DbgOut(_T("Failed to load %s - not a valid DLL file!"), Data.cFileName);
				continue;
			}
			ThisDLL->LoadDLL = (FLoadMapperDLL)GetProcAddress(ThisDLL->dInst, "LoadMapperDLL");
			ThisDLL->UnloadDLL = (FUnloadMapperDLL)GetProcAddress(ThisDLL->dInst, "UnloadMapperDLL");
			if ((ThisDLL->LoadDLL) && (ThisDLL->UnloadDLL))
			{
				ThisDLL->DI = ThisDLL->LoadDLL(hMainWnd, &EI, CurrentMapperInterface);
				if (ThisDLL->DI)
				{
					DbgOut(_T("Added mapper pack %s: '%s' v%X.%X (%04X/%02X/%02X)"), Data.cFileName, ThisDLL->DI->Description, ThisDLL->DI->Version >> 16, ThisDLL->DI->Version & 0xFFFF, ThisDLL->DI->Date >> 16, (ThisDLL->DI->Date >> 8) & 0xFF, ThisDLL->DI->Date & 0xFF);
					ThisDLL->Next = MapperDLLs;
					MapperDLLs = ThisDLL;
					ThisDLL = new MapperDLL;
				}
				else
				{
					DbgOut(_T("Failed to load mapper pack %s - version mismatch!"), Data.cFileName);
					FreeLibrary(ThisDLL->dInst);
				}
			}
			else
			{
				DbgOut(_T("Failed to load %s - not a valid mapper pack!"), Data.cFileName);
				FreeLibrary(ThisDLL->dInst);
			}
		}	while (FindNextFile(Handle, &Data));
		FindClose(Handle);
	}
	delete ThisDLL;
	if (MapperDLLs == NULL)
		MessageBox(hMainWnd, _T("Fatal error: unable to locate any mapper DLLs!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
#else	/* NSFPLAYER */
	DI = NULL;
	dInst = LoadLibrary(_T("Plugins\\nsf.dll"));
	if (dInst)
	{
		LoadDLL = (FLoadMapperDLL)GetProcAddress(dInst, "LoadMapperDLL");
		UnloadDLL = (FUnloadMapperDLL)GetProcAddress(dInst, "UnloadMapperDLL");
		if (LoadDLL && UnloadDLL)
		{
			DI = LoadDLL(mod.hMainWindow, &EI, CurrentMapperInterface);
			if (!DI)
			{
				MessageBox(mod.hMainWindow, _T("Fatal error: NSF.DLL reported version mismatch!"), _T("in_nintendulator"), MB_OK | MB_ICONERROR);
				FreeLibrary(dInst);
				dInst = NULL;
			}
		}
		else
		{
			MessageBox(mod.hMainWindow, _T("Fatal error: NSF.DLL is not a valid mapper pack!"), _T("in_nintendulator"), MB_OK | MB_ICONERROR);
			FreeLibrary(dInst);
			dInst = NULL;
		}
	}
	else	MessageBox(mod.hMainWindow, _T("Fatal error: unable to load NSF.DLL!"), _T("in_nintendulator"), MB_OK | MB_ICONERROR);
#endif	/* !NSFPLAYER */
	ZeroMemory(&EI, sizeof(EI));
	ZeroMemory(&RI, sizeof(RI));
	EI.SetCPUWriteHandler = SetCPUWriteHandler;
	EI.SetCPUReadHandler = SetCPUReadHandler;
	EI.GetCPUWriteHandler = GetCPUWriteHandler;
	EI.GetCPUReadHandler = GetCPUReadHandler;
	EI.SetPPUWriteHandler = SetPPUWriteHandler;
	EI.SetPPUReadHandler = SetPPUReadHandler;
	EI.GetPPUWriteHandler = GetPPUWriteHandler;
	EI.GetPPUReadHandler = GetPPUReadHandler;
	EI.SetPRG_ROM4 = SetPRG_ROM4;
	EI.SetPRG_ROM8 = SetPRG_ROM8;
	EI.SetPRG_ROM16 = SetPRG_ROM16;
	EI.SetPRG_ROM32 = SetPRG_ROM32;
	EI.GetPRG_ROM4 = GetPRG_ROM4;
	EI.SetPRG_RAM4 = SetPRG_RAM4;
	EI.SetPRG_RAM8 = SetPRG_RAM8;
	EI.SetPRG_RAM16 = SetPRG_RAM16;
	EI.SetPRG_RAM32 = SetPRG_RAM32;
	EI.GetPRG_RAM4 = GetPRG_RAM4;
	EI.GetPRG_Ptr4 = GetPRG_Ptr4;
	EI.SetPRG_Ptr4 = SetPRG_Ptr4;
	EI.SetPRG_OB4 = SetPRG_OB4;
	EI.SetCHR_ROM1 = SetCHR_ROM1;
	EI.SetCHR_ROM2 = SetCHR_ROM2;
	EI.SetCHR_ROM4 = SetCHR_ROM4;
	EI.SetCHR_ROM8 = SetCHR_ROM8;
	EI.GetCHR_ROM1 = GetCHR_ROM1;
	EI.SetCHR_RAM1 = SetCHR_RAM1;
	EI.SetCHR_RAM2 = SetCHR_RAM2;
	EI.SetCHR_RAM4 = SetCHR_RAM4;
	EI.SetCHR_RAM8 = SetCHR_RAM8;
	EI.GetCHR_RAM1 = GetCHR_RAM1;
	EI.SetCHR_NT1 = SetCHR_NT1;
	EI.GetCHR_NT1 = GetCHR_NT1;
	EI.GetCHR_Ptr1 = GetCHR_Ptr1;
	EI.SetCHR_Ptr1 = SetCHR_Ptr1;
	EI.SetCHR_OB1 = SetCHR_OB1;
	EI.Mirror_H = Mirror_H;
	EI.Mirror_V = Mirror_V;
	EI.Mirror_4 = Mirror_4;
	EI.Mirror_S0 = Mirror_S0;
	EI.Mirror_S1 = Mirror_S1;
	EI.Mirror_Custom = Mirror_Custom;
	EI.SetIRQ = SetIRQ;
	EI.Set_SRAMSize = Set_SRAMSize;
	EI.DbgOut = DbgOut;
	EI.StatusOut = StatusOut;
	EI.OpenBus = &CPU::LastRead;
	MI = NULL;
#ifndef	NSFPLAYER
	MI2 = NULL;
#endif	/* !NSFPLAYER */
}

#ifndef	NSFPLAYER
INT_PTR CALLBACK	DllSelect (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DLLInfo **DLLs = (DLLInfo **)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	int i;
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		DLLs = (DLLInfo **)lParam;
		for (i = 0; DLLs[i] != NULL; i++)
		{
			TCHAR *desc = new TCHAR[_tcslen(DLLs[i]->Description) + 32];
			_stprintf(desc, _T("\"%s\" v%x.%x (%04x/%02x/%02x)"),
				DLLs[i]->Description,
				(DLLs[i]->Version >> 16) & 0xFFFF, DLLs[i]->Version & 0xFFFF,
				(DLLs[i]->Date >> 16) & 0xFFFF, (DLLs[i]->Date >> 8) & 0xFF, DLLs[i]->Date & 0xFF);
			SendDlgItemMessage(hDlg, IDC_DLL_LIST, LB_INSERTSTRING, i, (LPARAM)desc);
			delete[] desc;
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_DLL_LIST:
			if (HIWORD(wParam) != LBN_DBLCLK)
				return TRUE;	// have double-clicks fall through
		case IDOK:
			i = SendDlgItemMessage(hDlg, IDC_DLL_LIST, LB_GETCURSEL, 0, 0);
			if (i == LB_ERR)
				EndDialog(hDlg, (INT_PTR)NULL);
			else	EndDialog(hDlg, (INT_PTR)DLLs[i]);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, (INT_PTR)NULL);
			return TRUE;
		}
	}
	return FALSE;
}
#endif	/* !NSFPLAYER */

BOOL	LoadMapper (const ROMInfo *ROM)
{
#ifndef	NSFPLAYER
	int num = 1;
	MapperDLL *ThisDLL = MapperDLLs;
	while (ThisDLL)
	{	// 1 - count how many DLLs we have (add 1 for null terminator)
		num++;
		ThisDLL = ThisDLL->Next;
	}
	// TODO - change this to keep track of the MapperInfo struct as well
	// so we don't need to LoadMapper, then UnloadMapper, then finally LoadMapper again
	DLLInfo **DLLs = new DLLInfo *[num];
	num = 0;
	ThisDLL = MapperDLLs;
	while (ThisDLL)
	{	// 2 - see how many DLLs support our mapper
		DI = ThisDLL->DI;
		if (DI->LoadMapper(ROM))
		{
			DI->UnloadMapper();
			DLLs[num++] = DI;
		}
		ThisDLL = ThisDLL->Next;
	}
	if (num == 0)
	{	// none found
		DI = NULL;
		delete[] DLLs;
		return FALSE;
	}
	if (num == 1)
	{	// 1 found
		DI = DLLs[0];
		delete[] DLLs;
		MI = DI->LoadMapper(ROM);
		// this should be impossible, since it worked just a moment ago
		if (!MI)
			return FALSE;
		if (MI->Load)
			return MI->Load();
		else	return TRUE;
	}
	// else more than one found
	DLLs[num] = NULL;
	DI = (DLLInfo *)DialogBoxParam(hInst, (LPCTSTR)IDD_DLLSELECT, hMainWnd, DllSelect, (LPARAM)DLLs);
	delete[] DLLs;
	if (DI)
	{
		MI = DI->LoadMapper(ROM);
		if (!MI)
			return FALSE;
		if (MI->Load)
			return MI->Load();
		else	return TRUE;
	}
	return FALSE;
#else	/* NSFPLAYER */
	if (!DI)
		return FALSE;
	MI = DI->LoadMapper(ROM);
	if (!MI)
		return FALSE;
	if (MI->Load)
		return MI->Load();
        else	return TRUE;
#endif	/* !NSFPLAYER */
}

void	UnloadMapper (void)
{
	if (MI)
	{
		if (MI->Unload)
			MI->Unload();
		MI = NULL;
	}
	if (DI)
	{
		if (DI->UnloadMapper)
			DI->UnloadMapper();
#ifndef	NSFPLAYER
		DI = NULL;
#endif	/* !NSFPLAYER */
	}
}

void	Release (void)
{
#ifndef	NSFPLAYER
	MapperDLL *ThisDLL = MapperDLLs;
	while (ThisDLL)
	{
		MapperDLLs = ThisDLL->Next;
		ThisDLL->UnloadDLL();
		FreeLibrary(ThisDLL->dInst);
		delete ThisDLL;
		ThisDLL = MapperDLLs;
	}
#else	/* NSFPLAYER */
	DI = NULL;
	UnloadDLL();
	FreeLibrary(dInst);
#endif	/* !NSFPLAYER */
}
} // namespace MapperInterface
