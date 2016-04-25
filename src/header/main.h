#pragma once

#ifndef __MODMAIN_H
#define __MODMAIN_H

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS 1

// let's do a precompiled header, why not
#pragma message( "Compiling precompiled header.\n" )

// handler not registered as safe handler
#pragma warning( disable : 4733 )

// API/SDK includes
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <shellapi.h>
#include <assert.h>
#include <algorithm>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>

#include "stdtypes.h"

/////////
#include <d3d9.h>
#include <d3dx9math.h>
#include <d3d9/d3d9proxy.h>
#include <d3d9/myIDirect3D9.h>
#include <d3d9/myIDirect3DDevice9.h>
#include <d3d9/cd3dfont.h>
/////////
#include <iostream>
#include <sstream>
#include <istream>
#include <ostream>
#include <fstream>
#include <iomanip>

extern myIDirect3DDevice9* gl_pmyIDirect3DDevice9;
extern myIDirect3D9*       gl_pmyIDirect3D9;
extern HINSTANCE           gl_hOriginalDll;
extern HINSTANCE           gl_hThisInstance;
extern CD3DRender			*render;

int process_init( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);

std::string string_format(const std::string fmt, ...) ;

//let the "hacking" begin :)

#define PI 3.1415926535897932384626433f

struct KeyManager
{
	bool Pressed;
	bool Down;
	bool Released;
	bool Up;
	bool ConsumePressed()
	{
		if(Pressed)
		{
			Pressed = false;
			return true;
		}
		return false;
	}
	bool ConsumeReleased()
	{
		if(Released)
		{
			Released = false;
			return true;
		}
		return false;
	}
	bool ConsumeDown()
	{
		if(Down)
		{
			Down = false;
			return true;
		}
		return false;
	}
	bool ConsumeUp()
	{
		if(Up)
		{
			Up = false;
			return true;
		}
		return false;
	}
};

extern KeyManager Keys[256];

void KeyManagerRun();

#define Keys(a) Keys[a]

#define PROTECT try{
#define UNPROTECT }catch(...){}
#define POINTER(type,addr) (*(type*)(addr))

namespace DirectXFont
{
	extern std::map<std::pair<std::string,std::pair<int,DWORD>>,CD3DFont> fonts;
	extern std::map<unsigned int,std::pair<std::string,std::pair<int,DWORD>>> font_id;
	extern unsigned int FontCounter;
	int Add(std::string fontname, int size, DWORD flags);
	bool Initialize(unsigned int ID);
	bool Remove(unsigned int ID);
	void InitializeAll();
	void InvalidateAll();
	CD3DFont * Access(int ID);
};

#endif
