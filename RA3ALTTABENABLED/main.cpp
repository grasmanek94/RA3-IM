#include <Winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
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
#include <thread>
#include <atomic>
#include <iostream>
#include <sstream>
#include <istream>
#include <ostream>
#include <fstream>
#include <iomanip>
#include <d3d9.h>
#include <d3dx9.h>
#include <d3dx9math.h>
#include <cd3dfont.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// global declarations
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class
HRESULT d3ddevice = NULL;
D3DPRESENT_PARAMETERS d3dpp;
int ScreenY = 768;
int ScreenX = 1024;
HWND ghWnd = NULL;

// function prototypes
void initD3D(HWND hWnd);    // sets up and initializes Direct3D
void render_frame(void);    // renders a single frame
void cleanD3D(void);    // closes Direct3D and releases memory
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);// the WindowProc function prototype
void OnRelease();
void OnInitialize(bool wait);
void KeyManagerRun();
void OnGameLaunch();
void texturesInitResources ( IDirect3DDevice9 *pDevice, bool init = true, bool whiledo = false);
void FrameTick(IDirect3DDevice9 * device, HWND wnd);

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "WindowClass";

    RegisterClassEx(&wc);

    hWnd = CreateWindowEx(NULL,
                          "WindowClass",
                          "Red Alert 3 Info Viewer",
                          WS_OVERLAPPEDWINDOW,
                          0, 0,
                          ScreenX, ScreenY,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);
	ghWnd = hWnd;
    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D
    initD3D(hWnd);

    // enter the main loop:

    MSG msg;

    while(TRUE)
    {
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(msg.message == WM_QUIT)
            break;

        render_frame();
    }

    // clean up DirectX and COM
    cleanD3D();

    return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
        {
			::exit(0);
            PostQuitMessage(0);
            return 0;
        } 
        case WM_SIZE:
        {
			// If the device is not NULL and the WM_SIZE message is not a
			// SIZE_MINIMIZED event, resize the device's swap buffers to match
			// the new window size.
			if( d3ddev != NULL && wParam != SIZE_MINIMIZED )
			{
				OnRelease();

				ScreenX					= LOWORD(lParam);	
				ScreenY					= HIWORD(lParam);
				d3dpp.BackBufferWidth  = LOWORD(lParam);
				d3dpp.BackBufferHeight = HIWORD(lParam);

				//if(
				d3ddev->Reset( &d3dpp )
                // == D3DERR_INVALIDCALL )
                //{
                   //error
                //}
				;
				OnInitialize(false);
			}
        }
		break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}

std::string string_format(const std::string fmt, ...) 
{
	int size = 512;
	std::string str;
	va_list ap;
	while (1) {
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char *)str.c_str(), size, fmt.c_str(), ap);
		va_end(ap);
		if (n > -1 && n < size) {
			str.resize(n);
			return str;
		}
		if (n > -1)
			size = n + 1;
		else
			size *= 2;
	}
	return str;
}

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

KeyManager Keys[256];

#define Keys(a) Keys[a]

#define PROTECT try{
#define UNPROTECT }catch(...){}
#define POINTER(type,addr) (*(type*)(addr))

#define MAX_PLAYERS (6)

namespace PINGER
{
	std::atomic<HANDLE> hIcmpFile[6];
	std::atomic<DWORD> PlayerPing[6];
	std::atomic<bool> running[2] = {false,false};
	void _trdpng(short id)
	{
		DWORD dwRetVal = 0;
		DWORD dwError = 0;
		char SendData[2] = {1,0};
		char ReplyBuffer[0xFFFF];
		DWORD ReplySize = sizeof (ICMP_ECHO_REPLY) + sizeof (SendData) + 8;
		while(running[0])
		{
			if(POINTER(DWORD,0x00400000+0x008E3A74) != NULL)
			{
				unsigned long ipaddr = (unsigned long)htonl(POINTER(unsigned long, POINTER(DWORD, POINTER(DWORD,0x00400000+0x008E3A74) + (0x04+(0x04*id)) ) +0x38));
				if(ipaddr != NULL)
				{
					dwRetVal = IcmpSendEcho2(hIcmpFile[id], NULL, NULL, NULL,
											 ipaddr, SendData, sizeof (SendData), NULL,
											 ReplyBuffer, ReplySize, 24900);
					if (dwRetVal != 0) 
					{
						PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY) ReplyBuffer;
						switch (pEchoReply->Status) 
						{
						case IP_DEST_HOST_UNREACHABLE:
							PlayerPing[id] = -3;
							break;
						case IP_DEST_NET_UNREACHABLE:
							PlayerPing[id] = -2;
							break;
						case IP_REQ_TIMED_OUT:
							PlayerPing[id] = -1;
							break;
						default:
							PlayerPing[id] = pEchoReply->RoundTripTime;
							break;
						}
					} 
					else 
					{
						dwError = GetLastError();
						switch (dwError) 
						{
						case IP_BUF_TOO_SMALL:
							PlayerPing[id] = -8;
							break;
						case IP_REQ_TIMED_OUT:
							PlayerPing[id] = -7;
							break;
						default:
							PlayerPing[id] = GetLastError()*-1;
							break;
						}
						
							//-4;
						//ERROR_INSUFFICIENT_BUFFER;
						//ERROR_INVALID_PARAMETER;
						//ERROR_NOT_ENOUGH_MEMORY;
						//ERROR_NOT_SUPPORTED;
						//IP_STATUS_BASE;
						//IP_BUF_TOO_SMALL;
					}
				}
				else
				{
					PlayerPing[id] = 0;
				}
			}
			Sleep(100);
		}
	}
	void MainThread()
	{
		running[1] = true;
		running[0] = true;
		for(short i = 0; i < 6; ++i)
		{
			hIcmpFile[i] = IcmpCreateFile();
			if (hIcmpFile[i] == INVALID_HANDLE_VALUE) 
			{
				running[1] = false;
				running[0] = false;
				for(short x = 0; x < i; ++x)
					IcmpCloseHandle(hIcmpFile[x]);
				return;
			}
		}
		std::thread pinger_0(_trdpng,0);
		std::thread pinger_1(_trdpng,1);
		std::thread pinger_2(_trdpng,2);
		std::thread pinger_3(_trdpng,3);
		std::thread pinger_4(_trdpng,4);
		std::thread pinger_5(_trdpng,5);
		pinger_0.join();
		pinger_1.join();
		pinger_2.join();
		pinger_3.join();
		pinger_4.join();
		pinger_5.join();
		running[1] = false;
	}
	bool start()
	{
		if(running[1])
			return false;
		std::thread main(MainThread);
		main.detach();
		return true;
	}
	bool IsRunning()
	{
		return (running[0] == true && running[1] == true);
	}
	bool stop()
	{
		if(!running[1])
			return false;
		running[0] = false;
		return true;
	}
	DWORD GetPing(short slot)
	{
		if(running[0])
		{
			if(slot < 0 || slot >= 6)
				return -6;
			return PlayerPing[slot];
		}
		return -5;
	}
};

struct RA3{
	static std::map<int,int> create_map()
	{
		std::map<int,int> m;
		m[-1]= 0xFFFFFFFF;
		m[0] = 0xFF324BC8;
		m[1] = 0xFFF0D719;
		m[2] = 0xFF14693C;
		m[3] = 0xFFE17314;
		m[4] = 0xFF7D19C8;
		m[5] = 0xFFE61414;
		m[6] = 0xFF66CFF8;
		return m;
	}
	static const std::map<int,int> PlayerColors;
};

struct pinfo
{
	bool			in;
	enum Controller	{Ctrl_UNKNOWN,	Ctrl_HUMAN,	Ctrl_AI				};
	enum Nation		{N_UNKNOWN,		N_ALLIES,	N_SOVIET,	N_JAPAN	};
	std::string		Name;
	int			Side;
	short			Team;
	int		Who;
	int				*Money;
	int				*Power;
	int				*Usage;
	unsigned int	*Color;
	int				*TotalAmountOfStuff;
	DWORD			*StuffPoolStart;

	bool			GotUnits;
	DWORD			*unit_start_node;
};

pinfo Player[MAX_PLAYERS];
bool GameInternsInited = false;

std::map<std::string,unsigned short> Locations;

struct StuffInfo
{
	int Side;
	bool structure;

	enum main_flags
	{
		antiair			= 0x00000400,
		antisurface		= 0x00001000
	};
	enum structure_flags
	{
		support				= 0x00000001,

		power				= 0x00000002,
		infantryproduction	= 0x00000004,
		vehicleproduction	= 0x00000008,
		aircraftproduction	= 0x00000010,
		resource			= 0x00000020,
		navyproduction		= 0x00000040,
		technology			= 0x00000080
	};
	enum unit_flags
	{
		amphibious		= 0x00000001,
		flying			= 0x00000002,

		scout			= 0x00000004,
		infantry		= 0x00000008,
		vehicle			= 0x00000010,
		aircraft		= 0x00000020,
		navy			= 0x00000040,

		antiinfantry	= 0x00000080,
		antigarrison	= 0x00000100,
		antiarmor		= 0x00000200,
		
		antiaircraft	= 0x00000800,
		antiship		= 0x00002000,

		fieldsupport	= 0x00004000,
		infiltrator		= 0x00008000,
		commando		= 0x00010000,
		transporter		= 0x00020000,
		resourcecollectr= 0x00040000,

		bombardment		= 0x00080000
	};
	int flags;
	StuffInfo()
	{
		Side = 0;
		flags = 0;
		structure = false;
	}
	StuffInfo(bool _structure, int _side = 0, int _flags = 0)
	{
		Side = _side;
		flags = _flags;
		structure = _structure;
	}
};

std::map<unsigned int,std::pair<unsigned short,StuffInfo>> StuffList;
const std::map<int,int> RA3:: PlayerColors =  RA3::create_map();

class Ra3Icon
{
public:
	Ra3Icon()//for std::map[]
	{
		texture = NULL;
		sprite = NULL;
		_set = false;
	}
	Ra3Icon(unsigned short ID)
	{
		toinit.assign(string_format("./d3d9dllsprites/%02x.png",ID));
		//toinit.assign(string_format("./d3d9dllsprites/DDS/%02x.dds",ID));
		texture = NULL;
		sprite = NULL;
		_set = true;
	}
	short Initialize(IDirect3DDevice9 *pDevice)
	{
		if(_set)
		{
			if(texture == NULL)
			{
				if(D3DXCreateTextureFromFileA( pDevice, toinit.c_str(), &texture ) != S_OK)
					return 1;
			}
			if(texture != NULL)
			{
				if(sprite == NULL)
				{
					if(D3DXCreateSprite( pDevice, &sprite ) == S_OK)
						return 0;
					return 2;
				}
				return 3;
			}
			return 4;
		}
		return 5;
	}
	bool Uninitialize()
	{
		if(_set)
		{
			if(sprite)
				sprite->Release();
			if(texture)
				texture->Release();
			sprite = NULL;
			texture = NULL;
			return true;
		}
		return false;
	}
	bool SetID(unsigned short ID)//for std::map[]
	{
		if(_set)
			return false;
		toinit.assign(string_format("./d3d9dllsprites/%02x.png",ID));
		//toinit.assign(string_format("./d3d9dllsprites/DDS/%02x.dds",ID));
		_set = true;
		return true;
	}
	bool Good()
	{
		if(_set && texture && sprite)
			return true;
		return false;
	}
	//here drawing...
	ULONG AddRef()
	{
		if(Good())
			return sprite->AddRef();
		return 0x7FFFFFFF;
	}
	HRESULT Begin(DWORD flags)
	{
		return Good() ? sprite->Begin(flags) : 0x7FFFFFFF;
	}
	HRESULT Draw(const RECT *pSrcRect,const D3DXVECTOR3 *pCenter,const D3DXVECTOR3 *pPosition,D3DCOLOR Color)
	{
		return Good() ? sprite->Draw(texture,pSrcRect,pCenter,pPosition,Color) : 0x7FFFFFFF;
	}
	HRESULT End()
	{
		return Good() ? sprite->End() : 0x7FFFFFFF;
	}
	HRESULT Flush()
	{
		return Good() ? sprite->Flush() : 0x7FFFFFFF;
	}
	HRESULT GetDevice(LPDIRECT3DDEVICE9 *ppDevice)
	{
		return Good() ? sprite->GetDevice(ppDevice) : 0x7FFFFFFF;
	}
	HRESULT GetTransform(D3DXMATRIX *pTransform)
	{
		return Good() ? sprite->GetTransform(pTransform) : 0x7FFFFFFF;
	}
	HRESULT OnLostDevice()
	{
		return Good() ? sprite->OnLostDevice() : 0x7FFFFFFF;
	}
	HRESULT OnResetDevice()
	{
		return Good() ? sprite->OnResetDevice() : 0x7FFFFFFF;
	}
	HRESULT QueryInterface(const IID &iid, LPVOID *ppv)
	{
		return Good() ? sprite->QueryInterface(iid,ppv) : 0x7FFFFFFF;
	}
	HRESULT SetTransform(D3DXMATRIX *pTransform)
	{
		return Good() ? sprite->SetTransform(pTransform) : 0x7FFFFFFF;
	}
	HRESULT SetWorldViewLH(D3DXMATRIX *pWorld,D3DXMATRIX *pView)
	{
		return Good() ? sprite->SetWorldViewLH(pWorld,pView) : 0x7FFFFFFF;
	}
	HRESULT SetWorldViewRH(D3DXMATRIX *pWorld,D3DXMATRIX *pView)
	{
		return Good() ? sprite->SetWorldViewRH(pWorld,pView) : 0x7FFFFFFF;
	}
private:
	bool				_set;
	std::string			toinit;
	IDirect3DTexture9	*texture;
	ID3DXSprite			*sprite;
};

std::map<unsigned short,Ra3Icon*> Icons;

unsigned int max_sessions = 1;

namespace DirectXFont
{
	std::map<std::pair<std::string,std::pair<int,DWORD>>,CD3DFont*> fonts;
	std::map<unsigned int,std::pair<std::string,std::pair<int,DWORD>>> font_id;
	unsigned int FontCounter = -1;
	int Add(std::string fontname, int size, DWORD flags)
	{
		if(fonts.find(std::make_pair(fontname,std::make_pair(size,flags))) == fonts.end())
		{
			for(auto it = font_id.begin(); it != font_id.end(); ++it)
			{
				if(it->second == std::make_pair(fontname,std::make_pair(size,flags)))
				{
					return it->first;//guaranteed to happen
				}
			}
		}
		fonts.emplace(std::make_pair(fontname,std::make_pair(size,flags)),new CD3DFont(fontname.c_str(),size,flags));
		font_id[++FontCounter] = std::make_pair(fontname,std::make_pair(size,flags));
		return FontCounter;
	}
	bool Initialize(unsigned int ID)
	{
		if(fonts.find(font_id[ID]) == fonts.end())
			return false;
		return fonts.at(font_id[ID])->Initialize( d3ddev ) == S_OK;
	}
	bool Remove(unsigned int ID)
	{
		if(fonts.find(font_id[ID]) == fonts.end())
			return false;
		fonts.at(font_id[ID])->Invalidate();
		fonts.erase(font_id[ID]);
		return true;
	}
	void InitializeAll()
	{
		for(auto it = fonts.begin(); it != fonts.end(); ++it)
			it->second->Initialize( d3ddev );
	}
	void InvalidateAll()
	{
		for(auto it = fonts.begin(); it != fonts.end(); ++it)
			it->second->Invalidate();
	}
	CD3DFont * Access(int ID)
	{
		if(ID < 0)
			return 0;
		if(fonts.find(font_id[ID]) == fonts.end())
			return 0;
		return fonts.at(font_id[ID]);
	}
};

void KeyManagerRun()
{
	for(unsigned int i = 0; i < 256; ++i)
	{
		if(GetAsyncKeyState(i))
		{
			if(!Keys[i].Down)
			{
				Keys[i].Down = true;
				Keys[i].Pressed = true;
				Keys[i].Released = false;
				Keys[i].Up = false;
			}
			else if(Keys[i].Pressed)
			{
				Keys[i].Pressed = false;
			}
		}
		else
		{
			if(!Keys[i].Up)
			{
				Keys[i].Released = true;
				Keys[i].Up = true;
				Keys[i].Down = false;
				Keys[i].Pressed = false;
			}
			else if(Keys[i].Released)
			{
				Keys[i].Released = false;
			}
		}
	}
}

CD3DRender				*render = new CD3DRender( 128 );

void OnInitialize(bool wait = false)
{
	OnGameLaunch();
	while(render->Initialize( d3ddev ) != S_OK/* && wait*/){}
	DirectXFont::Add("Lucida Console",10,FW_BOLD);
	DirectXFont::InitializeAll();
	texturesInitResources( d3ddev, true, wait );
}

void OnRelease()
{
	DirectXFont::InvalidateAll();
	texturesInitResources( d3ddev, false );
	render->Invalidate();
}

inline bool IsPlayerInGame()
{
	return (POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+0xD8) != NULL);
}

void texturesInitResources ( IDirect3DDevice9 *pDevice, bool init, bool whiledo)
{
	static bool Created = false;
	std::map<short,unsigned int> errors;
	if(init)
	{
		for(auto it = Locations.begin(); it != Locations.end(); ++it)
		{
			if(!Created)
				Icons.emplace(std::pair<unsigned short,Ra3Icon*>(it->second,new Ra3Icon(it->second)));
			short ret = Icons[it->second]->Initialize(pDevice);
			if(whiledo)
			{
				while(ret)
					ret = Icons[it->second]->Initialize(pDevice);
			}
			++errors[ret];
		}
		Created = true;
		/*std::stringstream ss;
		if((errors[1] + errors[2] + errors[3] + errors[4] + errors[5]) > 0)
		{
			ss << "Failed to initialize " << (errors[1] + errors[2] + errors[3] + errors[4] + errors[5]) << " icons:\r\n";
			ss << "\tType 1: " << errors[1] << "\r\n";
			ss << "\tType 2: " << errors[2] << "\r\n";
			ss << "\tType 3: " << errors[3] << "\r\n";
			ss << "\tType 4: " << errors[4] << "\r\n";
			ss << "\tType 5: " << errors[5] << "\r\n";

			MessageBox(ghWnd,ss.str().c_str(),"ERROR",0);
		}*/
	}
	else
	{
		for(auto it = Locations.begin(); it != Locations.end(); ++it)
			Icons[it->second]->Uninitialize();
	}
}

void OnGameLaunch()
{
	if(GameInternsInited)
		return;

	GameInternsInited = true;
	///////texture info
	Locations[""] = 0x00;
	//Allies
	Locations["Power Plant"] = 0x90;
	Locations["Bootcamp"] = 0x01;
	Locations["Allied Ore Refinery"] = 0x02;
	Locations["Spectrum Tower"] = 0x03;
	Locations["Attack Dog"] = 0x04;
	Locations["Allied Engineer"] = 0x05;
	Locations["Prospector"] = 0x06;
	Locations["Guardian Tank"] = 0x07;
	Locations["Vindicator"] = 0x08;
	Locations["Century Bomber"] = 0x09;
	Locations["Assault Destroyer"] = 0x0A;
	Locations["Armor Factory"] = 0x10;
	Locations["Seaport"] = 0x11;
	Locations["Airbase"] = 0x12;
	Locations["Chronosphere"] = 0x13;
	Locations["Peacekeeper"] = 0x14;
	Locations["Spy"] = 0x15;
	Locations["Raptide ACV"] = 0x16;
	Locations["Athena Cannon"] = 0x17;
	Locations["Apollo Fighter"] = 0x18;
	Locations["Dolphin"] = 0x19;
	Locations["Aircraft Carrier"] = 0x1A;
	Locations["Defense Bureau"] = 0x20;
	Locations["Allied Wall"] = 0x21;
	Locations["Multigunner Turret"] = 0x22;
	Locations["Proton Collider"] = 0x23;
	Locations["Javelin Soldier"] = 0x24;
	Locations["Tanya"] = 0x25;
	Locations["Multigunner IFV"] = 0x26;
	Locations["Mirage Tank"] = 0x27;
	Locations["Hydrofoil"] = 0x28;
	Locations["Cryocopter"] = 0x29;
	Locations["Allied MCV"] = 0x2A;

	//Japan
	Locations["Instant Generator"] = 0x30;
	Locations["Instant Dojo"] = 0x31;
	Locations["Japan Ore Refinery"] = 0x32;
	Locations["Japan Wall"] = 0x33;
	Locations["Burst Drone"] = 0x34;
	Locations["Tankbuster"] = 0x35;
	Locations["Imperial Warrior"] = 0x36;
	Locations["Japan Ore Collector"] = 0x37;
	Locations["Sudden Transport"] = 0x38;
	Locations["Mecha Tengu"] = 0x39;
	Locations["Mecha Bay"] = 0x40;
	Locations["Imperial Docks"] = 0x41;
	Locations["Nanotech Mainframe"] = 0x42;
	Locations["Defender-VX"] = 0x43;
	Locations["Japan Engineer"] = 0x44;
	Locations["Rocket Angle"] = 0x45;
	Locations["Shinobi"] = 0x46;
	Locations["Tsunami Tank"] = 0x47;
	Locations["Striker-VX"] = 0x48;
	Locations["King Oni"] = 0x49;
	Locations["Nanoswarm Hive"] = 0x50;
	Locations["Psionic Decimator"] = 0x51;
	Locations["Wave-Force Tower"] = 0x52;
	Locations["Yar Mini-Sub"]		= 0x53;
	Locations["Yuriko Omega"]		= 0x54;
	Locations["Sea-Wing"]			= 0x55;
	Locations["Naginata Cruiser"]	= 0x56;
	Locations["Wave-Force Artillery"] = 0x57;
	Locations["Japan MCV"]			= 0x58;
	Locations["Shogun Battleship"]	= 0x59;

	Locations["Tower Core"]			= 0x91;
	Locations["Defender Core"]		= 0x92;
	Locations["Decimator Core"]		= 0x93;
	Locations["Nanoswarm Core"]		= 0x94;
	Locations["Mainframe Core"]		= 0x95;
	Locations["Refinery Core"]		= 0x96;
	Locations["Dojo Core"]			= 0x97;
	Locations["Docks Core"]			= 0x98;
	Locations["Mecha Bay Core"]		= 0x99;
	Locations["Generator Core"]		= 0x9A;

	//Soviet
	Locations["Hammer Tank"]		= 0x4A;
	Locations["Terror Drone"]		= 0x3A;
	Locations["Soviet MCV"]			= 0x5A;
	Locations["Reactor"]			= 0x60;
	Locations["Barracks"]			= 0x61;
	Locations["Soviet Ore Refinery"] = 0x62;
	Locations["Soviet Wall"]		= 0x63;
	Locations["Sentry Gun"]			= 0x64;
	Locations["War Bear"]			= 0x65;
	Locations["Flak Cannon"]		= 0x66;
	Locations["Conscript"]			= 0x67;
	Locations["Flak Trooper"]		= 0x68;
	Locations["Soviet Ore Collector"] = 0x69;
	Locations["Sputnik"]			= 0x6A;
	Locations["War Factory"]		= 0x70;
	Locations["Naval Yard"]			= 0x71;
	Locations["Airfield"]			= 0x72;
	Locations["Tesla Coil"]			= 0x73;
	Locations["Iron Curtain"]		= 0x74;
	Locations["Soviet Engineer"]	= 0x75;
	Locations["Vacuum Imploder"]	= 0x76;
	Locations["Tesla Trooper"]		= 0x77;
	Locations["Natasha"]			= 0x78;
	Locations["Sickle"]				= 0x79;
	Locations["Bullfrog"]			= 0x7A;
	Locations["Super-Reactor"]		= 0x80;
	Locations["Battle Lab"]			= 0x81;
	Locations["Crusher Crane"]		= 0x82;
	Locations["Stingray"]			= 0x83;
	Locations["Akula Sub"]			= 0x84;
	Locations["Twinblade"]			= 0x85;
	Locations["Dreadnought"]		= 0x86;
	Locations["MiG Fighter"]		= 0x87;
	Locations["Kirov Airship"]		= 0x88;
	Locations["V4 Rocket Launcher"] = 0x89;
	Locations["Apocalypse Tank"]	= 0x8A;

	//
	Locations["INTERN_ALLIED"]		= 0xA2;
	Locations["INTERN_JAPAN"]		= 0xA3;
	Locations["INTERN_SOVIET"]		= 0xA4;
	Locations["INTERN_CIRCLE"]		= 0xA5;
	Locations["INTERN_SQUARE"]		= 0xA6;
	Locations["INTERN_TRIANGLE"]	= 0xA7;
	Locations["INTERN_CROSS"]		= 0xA8;
	Locations["INTERN_LINE"]		= 0xA9;
	Locations["INTERN_XCROSS"]		= 0xAA;
	Locations["INTERN_BOX"]			= 0xAB;

	//
	Locations["Allied Construction Yard"]			= 0x0B;
	Locations["Soviet Construction Yard"]			= 0x0C;
	Locations["Japan Construction Yard"]			= 0x0D;

	//
	StuffList[0x00000000] = std::pair<unsigned short,StuffInfo>(0x00,StuffInfo());
	StuffList[0x509BD329] = std::pair<unsigned short,StuffInfo>(0x01,StuffInfo());
	StuffList[0x8ECE261C] = std::pair<unsigned short,StuffInfo>(0x02,StuffInfo());
	StuffList[0x69B62705] = std::pair<unsigned short,StuffInfo>(0x03,StuffInfo());
	StuffList[0xDDFC28DE] = std::pair<unsigned short,StuffInfo>(0x04,StuffInfo());
	StuffList[0xE1E9179B] = std::pair<unsigned short,StuffInfo>(0x05,StuffInfo());
	StuffList[0x2A196E71] = std::pair<unsigned short,StuffInfo>(0x06,StuffInfo());
	StuffList[0x75288D70] = std::pair<unsigned short,StuffInfo>(0x06,StuffInfo());
	StuffList[0x07B91527] = std::pair<unsigned short,StuffInfo>(0x07,StuffInfo());
	StuffList[0xB74F8348] = std::pair<unsigned short,StuffInfo>(0x08,StuffInfo());
	StuffList[0x83D5A86B] = std::pair<unsigned short,StuffInfo>(0x09,StuffInfo());
	StuffList[0x5AE534FC] = std::pair<unsigned short,StuffInfo>(0x0A,StuffInfo());
	StuffList[0x8209C058] = std::pair<unsigned short,StuffInfo>(0x10,StuffInfo());
	StuffList[0x7848F598] = std::pair<unsigned short,StuffInfo>(0x11,StuffInfo());
	StuffList[0x5B3008B7] = std::pair<unsigned short,StuffInfo>(0x12,StuffInfo());
	StuffList[0xFD87E82A] = std::pair<unsigned short,StuffInfo>(0x13,StuffInfo());
	StuffList[0x139CBC97] = std::pair<unsigned short,StuffInfo>(0x14,StuffInfo());
	StuffList[0x4AA5D515] = std::pair<unsigned short,StuffInfo>(0x15,StuffInfo());
	StuffList[0x28DA574E] = std::pair<unsigned short,StuffInfo>(0x16,StuffInfo());
	StuffList[0x648D1440] = std::pair<unsigned short,StuffInfo>(0x16,StuffInfo());
	StuffList[0xD48ED838] = std::pair<unsigned short,StuffInfo>(0x17,StuffInfo());
	StuffList[0x3C82B910] = std::pair<unsigned short,StuffInfo>(0x18,StuffInfo());
	StuffList[0x8ACA3F75] = std::pair<unsigned short,StuffInfo>(0x19,StuffInfo());
	StuffList[0x09705D80] = std::pair<unsigned short,StuffInfo>(0x1A,StuffInfo());
	StuffList[0xCA9257EB] = std::pair<unsigned short,StuffInfo>(0x20,StuffInfo());
	StuffList[0x09435832] = std::pair<unsigned short,StuffInfo>(0x21,StuffInfo());
	StuffList[0x296799CF] = std::pair<unsigned short,StuffInfo>(0x21,StuffInfo());
	StuffList[0xEE3E07BD] = std::pair<unsigned short,StuffInfo>(0x22,StuffInfo());
	StuffList[0x95D6E965] = std::pair<unsigned short,StuffInfo>(0x23,StuffInfo());
	StuffList[0x9C5D3BB8] = std::pair<unsigned short,StuffInfo>(0x24,StuffInfo());
	StuffList[0x53E0EB12] = std::pair<unsigned short,StuffInfo>(0x25,StuffInfo());
	StuffList[0xBB06395A] = std::pair<unsigned short,StuffInfo>(0x26,StuffInfo());
	StuffList[0x52BFE9C5] = std::pair<unsigned short,StuffInfo>(0x27,StuffInfo());
	StuffList[0x2E211A99] = std::pair<unsigned short,StuffInfo>(0x28,StuffInfo());
	StuffList[0x509D5101] = std::pair<unsigned short,StuffInfo>(0x29,StuffInfo());
	StuffList[0x4068B3D7] = std::pair<unsigned short,StuffInfo>(0x2A,StuffInfo());
	StuffList[0x1C331EB6] = std::pair<unsigned short,StuffInfo>(0x2A,StuffInfo());
	StuffList[0x309480DD] = std::pair<unsigned short,StuffInfo>(0x30,StuffInfo());
	StuffList[0x04F32965] = std::pair<unsigned short,StuffInfo>(0x31,StuffInfo());
	StuffList[0x5D02B20C] = std::pair<unsigned short,StuffInfo>(0x32,StuffInfo());
	StuffList[0xBF93CE00] = std::pair<unsigned short,StuffInfo>(0x33,StuffInfo());
	StuffList[0xF8C50039] = std::pair<unsigned short,StuffInfo>(0x33,StuffInfo());
	StuffList[0x12E7D7A4] = std::pair<unsigned short,StuffInfo>(0x34,StuffInfo());
	StuffList[0x7D0549DD] = std::pair<unsigned short,StuffInfo>(0x35,StuffInfo());
	StuffList[0xDDACA5DD] = std::pair<unsigned short,StuffInfo>(0x35,StuffInfo());
	StuffList[0x0FB02C55] = std::pair<unsigned short,StuffInfo>(0x36,StuffInfo());
	StuffList[0x92CDE50F] = std::pair<unsigned short,StuffInfo>(0x37,StuffInfo());
	StuffList[0xE27A88A0] = std::pair<unsigned short,StuffInfo>(0x37,StuffInfo());
	StuffList[0x1791E072] = std::pair<unsigned short,StuffInfo>(0x38,StuffInfo());
	StuffList[0xC3986ED4] = std::pair<unsigned short,StuffInfo>(0x39,StuffInfo());
	StuffList[0xD741D327] = std::pair<unsigned short,StuffInfo>(0x3A,StuffInfo());
	StuffList[0xA689B39A] = std::pair<unsigned short,StuffInfo>(0x40,StuffInfo());
	StuffList[0x263DA9AF] = std::pair<unsigned short,StuffInfo>(0x41,StuffInfo());
	StuffList[0xD20CC8DE] = std::pair<unsigned short,StuffInfo>(0x42,StuffInfo());
	StuffList[0x89AE48EA] = std::pair<unsigned short,StuffInfo>(0x43,StuffInfo());
	StuffList[0x20DBDFCC] = std::pair<unsigned short,StuffInfo>(0x44,StuffInfo());
	StuffList[0xD81C1012] = std::pair<unsigned short,StuffInfo>(0x45,StuffInfo());
	StuffList[0xA768E216] = std::pair<unsigned short,StuffInfo>(0x46,StuffInfo());
	StuffList[0xCEB7DA1F] = std::pair<unsigned short,StuffInfo>(0x47,StuffInfo());
	StuffList[0xBA8E535F] = std::pair<unsigned short,StuffInfo>(0x47,StuffInfo());
	StuffList[0x6E8C5FFF] = std::pair<unsigned short,StuffInfo>(0x48,StuffInfo());
	StuffList[0x59908E62] = std::pair<unsigned short,StuffInfo>(0x49,StuffInfo());
	StuffList[0x94B3590B] = std::pair<unsigned short,StuffInfo>(0x4A,StuffInfo());
	StuffList[0xD6D22475] = std::pair<unsigned short,StuffInfo>(0x50,StuffInfo());
	StuffList[0x1AFC9A6E] = std::pair<unsigned short,StuffInfo>(0x51,StuffInfo());
	StuffList[0x2D795EEA] = std::pair<unsigned short,StuffInfo>(0x52,StuffInfo());
	StuffList[0xCEDA61FD] = std::pair<unsigned short,StuffInfo>(0x53,StuffInfo());
	StuffList[0x6586A5A0] = std::pair<unsigned short,StuffInfo>(0x54,StuffInfo());
	StuffList[0xB142DEC6] = std::pair<unsigned short,StuffInfo>(0x55,StuffInfo());
	StuffList[0x1A2CA9AB] = std::pair<unsigned short,StuffInfo>(0x56,StuffInfo());
	StuffList[0x90B81D3C] = std::pair<unsigned short,StuffInfo>(0x57,StuffInfo());
	StuffList[0x1C2EF767] = std::pair<unsigned short,StuffInfo>(0x58,StuffInfo());
	StuffList[0xD2ECDA2C] = std::pair<unsigned short,StuffInfo>(0x58,StuffInfo());
	StuffList[0xFC5E3314] = std::pair<unsigned short,StuffInfo>(0x59,StuffInfo());
	StuffList[0xAF4C0DA5] = std::pair<unsigned short,StuffInfo>(0x5A,StuffInfo());
	StuffList[0x1545FAC2] = std::pair<unsigned short,StuffInfo>(0x5A,StuffInfo());
	StuffList[0xCF8D9F20] = std::pair<unsigned short,StuffInfo>(0x60,StuffInfo());
	StuffList[0x80D4EA1E] = std::pair<unsigned short,StuffInfo>(0x61,StuffInfo());
	StuffList[0xC45AAEAB] = std::pair<unsigned short,StuffInfo>(0x62,StuffInfo());
	StuffList[0x0895CAE6] = std::pair<unsigned short,StuffInfo>(0x63,StuffInfo());
	StuffList[0xA82CF003] = std::pair<unsigned short,StuffInfo>(0x63,StuffInfo());
	StuffList[0x4DF5F9C1] = std::pair<unsigned short,StuffInfo>(0x64,StuffInfo());
	StuffList[0x33026FA1] = std::pair<unsigned short,StuffInfo>(0x65,StuffInfo());
	StuffList[0x1769BE29] = std::pair<unsigned short,StuffInfo>(0x66,StuffInfo());
	StuffList[0x19AB11AF] = std::pair<unsigned short,StuffInfo>(0x67,StuffInfo());
	StuffList[0x2C358C61] = std::pair<unsigned short,StuffInfo>(0x68,StuffInfo());
	StuffList[0x951764B9] = std::pair<unsigned short,StuffInfo>(0x69,StuffInfo());
	StuffList[0xF6DAC2A4] = std::pair<unsigned short,StuffInfo>(0x69,StuffInfo());
	StuffList[0x92718B35] = std::pair<unsigned short,StuffInfo>(0x6A,StuffInfo());
	StuffList[0xBB686AE1] = std::pair<unsigned short,StuffInfo>(0x6A,StuffInfo());
	StuffList[0xD50C222E] = std::pair<unsigned short,StuffInfo>(0x70,StuffInfo());
	StuffList[0x6E1ABB35] = std::pair<unsigned short,StuffInfo>(0x71,StuffInfo());
	StuffList[0xD25BB962] = std::pair<unsigned short,StuffInfo>(0x72,StuffInfo());
	StuffList[0xFA1F6466] = std::pair<unsigned short,StuffInfo>(0x73,StuffInfo());
	StuffList[0x3E02A960] = std::pair<unsigned short,StuffInfo>(0x73,StuffInfo());
	StuffList[0xD20552E1] = std::pair<unsigned short,StuffInfo>(0x74,StuffInfo());
	StuffList[0xA08AABD3] = std::pair<unsigned short,StuffInfo>(0x75,StuffInfo());
	StuffList[0x856C9DD6] = std::pair<unsigned short,StuffInfo>(0x76,StuffInfo());
	StuffList[0x3E11865D] = std::pair<unsigned short,StuffInfo>(0x77,StuffInfo());
	StuffList[0x174F874F] = std::pair<unsigned short,StuffInfo>(0x78,StuffInfo());
	StuffList[0x60F0B4E9] = std::pair<unsigned short,StuffInfo>(0x79,StuffInfo());
	StuffList[0xCCBD2C91] = std::pair<unsigned short,StuffInfo>(0x7A,StuffInfo());
	StuffList[0x522BCB61] = std::pair<unsigned short,StuffInfo>(0x7A,StuffInfo());
	StuffList[0x524BBD01] = std::pair<unsigned short,StuffInfo>(0x80,StuffInfo());
	StuffList[0xB7377639] = std::pair<unsigned short,StuffInfo>(0x81,StuffInfo());
	StuffList[0xA01D437D] = std::pair<unsigned short,StuffInfo>(0x82,StuffInfo());
	StuffList[0xAFF62E78] = std::pair<unsigned short,StuffInfo>(0x83,StuffInfo());
	StuffList[0x8D0A384A] = std::pair<unsigned short,StuffInfo>(0x84,StuffInfo());
	StuffList[0x40ACAD6D] = std::pair<unsigned short,StuffInfo>(0x85,StuffInfo());
	StuffList[0xBCEF51B9] = std::pair<unsigned short,StuffInfo>(0x86,StuffInfo());
	StuffList[0x7F54CED1] = std::pair<unsigned short,StuffInfo>(0x87,StuffInfo());
	StuffList[0xFFA811AE] = std::pair<unsigned short,StuffInfo>(0x88,StuffInfo());
	StuffList[0x821381DE] = std::pair<unsigned short,StuffInfo>(0x89,StuffInfo());
	StuffList[0x9E45383C] = std::pair<unsigned short,StuffInfo>(0x8A,StuffInfo());
	StuffList[0x89B86E3D] = std::pair<unsigned short,StuffInfo>(0x90,StuffInfo());
	StuffList[0x2412FB82] = std::pair<unsigned short,StuffInfo>(0x91,StuffInfo());
	StuffList[0x1F496B6B] = std::pair<unsigned short,StuffInfo>(0x92,StuffInfo());
	StuffList[0x5C1C0F0F] = std::pair<unsigned short,StuffInfo>(0x93,StuffInfo());
	StuffList[0x5FC93021] = std::pair<unsigned short,StuffInfo>(0x94,StuffInfo());
	StuffList[0x78D3E32C] = std::pair<unsigned short,StuffInfo>(0x95,StuffInfo());
	StuffList[0xFF4A8B60] = std::pair<unsigned short,StuffInfo>(0x96,StuffInfo());
	StuffList[0x57F07A3A] = std::pair<unsigned short,StuffInfo>(0x97,StuffInfo());
	StuffList[0xF0455D99] = std::pair<unsigned short,StuffInfo>(0x98,StuffInfo());
	StuffList[0xA02E31D3] = std::pair<unsigned short,StuffInfo>(0x99,StuffInfo());
	StuffList[0xC6B3F8AA] = std::pair<unsigned short,StuffInfo>(0x9A,StuffInfo());
	StuffList[0x0C3040FF] = std::pair<unsigned short,StuffInfo>(0xA0,StuffInfo());
	StuffList[0x09442731] = std::pair<unsigned short,StuffInfo>(0xA1,StuffInfo());

	StuffList[0x61A093F6] = std::pair<unsigned short,StuffInfo>(0x0B,StuffInfo());
	StuffList[0xC16FF13F] = std::pair<unsigned short,StuffInfo>(0x0C,StuffInfo());
	StuffList[0x356FA3A9] = std::pair<unsigned short,StuffInfo>(0x0D,StuffInfo());
}
//
class Timer
{
public:
    // Construct the timer and initialize all of the
    // members by resetting them to their zero-values.
    Timer(void) {
        Reset();
    }
     
    ~Timer(void){ /* empty */ }
  
    // Activate the timer and poll the counter.
    void Start(void) {
        _Active = true;
        PollCounter(_StartCount);
    }
     
    // Deactivate the timer and poll the counter.
    void Stop(void) {
        _Active = false;
        PollCounter(_EndCount);
    }
     
    // Stops the timer if it's active and resets all
    // of the Timer's members to their initial values.
    void Reset(void) {
        if (_Active)
            Stop();
         
        QueryPerformanceFrequency(&_Frequency);
         
        _StartTime = (_EndTime = 0.0);
        _StartCount.QuadPart = (_EndCount.QuadPart = 0);
        _Active = false;
    }
     
    // Returns the time elapsed since Start() was called
    // in micro-seconds
    const long double GetTimeInMicroSeconds(void) {
        const long double MicroFreq =
            1000000.0 / _Frequency.QuadPart;
         
        if (_Active)
            PollCounter(_EndCount);
         
         
        _StartTime = _StartCount.QuadPart * MicroFreq;
        _EndTime = _EndCount.QuadPart * MicroFreq;
         
        return _EndTime - _StartTime;
    }
     
    // Returns the time elapsed since Start() was called
    // in milli-seconds
    const long double GetTimeInMilliseconds(void) {
        return GetTimeInMicroSeconds() * 0.001;
    }
     
    // Returns the time elapsed since Start() was called
    // in seconds
    const long double GetTimeInSeconds( void ) {
        return GetTimeInMicroSeconds() * 0.000001;
    }
     
    // Returns TRUE if the Timer is currently active
    const bool IsActive(void) const {
        return _Active;
    }
private:
    // Poll the query performance counter, safely by tying
    // the polling functionality temporarily to a single
    // logical processor (identified by 0).
    void PollCounter(LARGE_INTEGER& Out) {
        HANDLE Thread = GetCurrentThread();
 
        DWORD_PTR OldMask = SetThreadAffinityMask(Thread, 0);
            QueryPerformanceCounter(&Out);
        SetThreadAffinityMask(Thread, OldMask);
    }
     
private:
    bool _Active;
    long double
        _StartTime,
        _EndTime;
    LARGE_INTEGER
        _Frequency,
        _StartCount,
        _EndCount;
}; // class Timer

Timer * myTime;
// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
	myTime = new Timer;
	myTime->Start();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

        // create a struct to hold various device information

    //ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;    // discard old frames
    d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D


    // create a device class using this information and the info from the d3dpp stuct
    d3ddevice = d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3ddev);
	OnInitialize(true);
}
//

// this is the function used to render a single frame
void render_frame(void)
{
	static double last_time = 0.0;
	double now_time = myTime->GetTimeInMilliseconds();

	if((now_time-last_time) >= 33.333333)//~30 FPS limit
	{
		last_time = now_time;
		// clear the window to a [BLACK]
		d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

		d3ddev->BeginScene();// begins the 3D scene
	
		// do 3D rendering on the back buffer here
		FrameTick(d3ddev,ghWnd);    	

		d3ddev->EndScene();// ends the 3D scene
		d3ddev->Present(NULL, NULL, NULL, NULL);   // displays the created frame on the screen
	}
}


// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
	OnRelease();
    d3ddev->Release();    // close and release the 3D device
    d3d->Release();    // close and release Direct3D
}

//////
void FrameTick(IDirect3DDevice9 * device, HWND wnd)
{
	PROTECT;
	KeyManagerRun();//key press/release detection
	if(Keys(VK_MENU).Down && Keys(VK_F4).Pressed)
		::exit(0);

	static bool got_session = false;

	if(!IsPlayerInGame())
	{
		if(got_session)
		{
			++max_sessions;
			got_session = false;
			for(short i = 0; i < MAX_PLAYERS; ++i)
			{
				Player[i].in = false;
				Player[i].GotUnits = false;
				Player[i].Money = NULL;
				Player[i].Power = NULL;
				Player[i].Color = NULL;
				Player[i].Usage = NULL;
				Player[i].TotalAmountOfStuff = NULL;
				Player[i].StuffPoolStart = NULL;
				Player[i].unit_start_node = NULL;
			}
		}
		if(POINTER(DWORD,0x00400000+0x008E3A74) != NULL)
		{
			if(PINGER::IsRunning())
			{
				for(short i = 0; i < 6; ++i)
				{
					DirectXFont::Access(0)->Print(0.0f,0.0f+(15.0f*((float)i)),-1,string_format("Player(%d) PING: %d",i,PINGER::GetPing(i) ).c_str(),false);
				}
			}
			else
			{
				PINGER::start();
			}
		}
		else
		{
			if(PINGER::IsRunning())
			{
				PINGER::stop();
			}
		}
		return;
	}
	if(PINGER::IsRunning())
	{
		PINGER::stop();
	}
	if(!got_session)
	{
		for(unsigned int i = max_sessions; ; ++i)
		{
			PROTECT;
			unsigned int adder = (0x4 * (i - 1));
			auto& Tester = POINTER(char,POINTER(DWORD,POINTER(DWORD,0x00CE3358)+adder)+0xF);
			if(Tester == '#')
			{
				char * GameSessionInfo = (POINTER(char*,POINTER(DWORD,0x00CE3358)+adder)+0x10);
				std::stringstream infoconvert(GameSessionInfo);
				std::string process;
				short mainslotsmin = 0;
				while(std::getline(infoconvert,process))
				{
					unsigned int WhatSlot = process.find("Slot ");
					if (WhatSlot != std::string::npos)
					{
						short slot = (process[WhatSlot+5]-48)-mainslotsmin;
						pinfo::Nation tempNation;
						unsigned int Nation = process.find("(Rising Sun),");
						if(Nation != std::string::npos)
						{
							tempNation = pinfo::Nation::N_JAPAN;
						}
						else
						{
							Nation = process.find("(Allies),"); 
							if(Nation != std::string::npos)
							{
								tempNation = pinfo::Nation::N_ALLIES;
							}
							else 
							{
								Nation = process.find("(Soviets),"); 
								if(Nation != std::string::npos)
								{
									tempNation = pinfo::Nation::N_SOVIET;
								}
								else
								{
									tempNation = pinfo::Nation::N_UNKNOWN;
									++mainslotsmin;
								}
							}
						}
						if(tempNation != pinfo::Nation::N_UNKNOWN)
						{
							unsigned int WhatController = process.find("Human",Nation);
							pinfo::Controller tempCtrl = pinfo::Controller::Ctrl_UNKNOWN;
							if (WhatController != std::string::npos)
								tempCtrl = pinfo::Controller::Ctrl_HUMAN;
							else
								tempCtrl = pinfo::Controller::Ctrl_AI;
							Player[slot].in = true;
							Player[slot].Side = tempNation;
							Player[slot].Who = tempCtrl;
							unsigned int start = WhatSlot+8;
							unsigned int amount = Nation - start;
							Player[slot].Name.assign(process.substr(start,amount));
							unsigned int TeamPos = process.find("Team:",Nation);
							if(process[TeamPos+5] == '-')
							{
								Player[slot].Team = -1;
							}
							else
							{
								Player[slot].Team = process[TeamPos+5]-48;
							}
							unsigned int CurrentSlot = (0xD8+(0x4*slot));
							if(POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+CurrentSlot) != NULL)
							{
								Player[slot].Color = &POINTER(unsigned int,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+CurrentSlot)+0x40)+0x0)+0x40);
								Player[slot].Money = &POINTER(int,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+CurrentSlot)+0xE4)+0x0)+0x4);
								Player[slot].Power = &POINTER(int,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+CurrentSlot)+0x8)+0x6C)+0x4);
								Player[slot].Usage = &POINTER(int,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+CurrentSlot)+0x8)+0x6C)+0x8);
								Player[slot].unit_start_node = &POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+CurrentSlot) + 0x38) + 0x0C);
								Player[slot].TotalAmountOfStuff = &POINTER(int,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E2F9C)+0x28)+0x40+(slot*0x10))+0x8);
								//Player[slot].StuffPoolStart = &POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+CurrentSlot)+0x444);
								Player[slot].StuffPoolStart = &POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+(CurrentSlot))+0x40);
							}
						}
					}
				}
				max_sessions = i;
				got_session = true;
			}
			else
			{
				break;
			}
			UNPROTECT;
		}
	}

	D3DXVECTOR2 CurrRes			(	(float)ScreenX									,	(float)ScreenY										);
	D3DXMATRIX mat;

	float PlayerSpacing = (CurrRes.y/6.0f < 80.0f) ? 80.0f : (CurrRes.y/6.0f);
	for(int player = 0; player < MAX_PLAYERS; ++player)
	{	
		if(Player[player].in)
		{
			float findex = (float)player;
			std::map<unsigned short,unsigned int> Stuff;

			//draw nation
			D3DXMatrixTransformation2D( &mat, NULL, 0.0f, &D3DXVECTOR2(40.0f/64.0f,40.0f/64.0f), NULL, 0.0f, &D3DXVECTOR2(0.0f, (PlayerSpacing*findex)+15.0f) );			
			
			PROTECT;
			switch(Player[player].Side)
			{
				case pinfo::Nation::N_ALLIES:
				{
					Icons[Locations["INTERN_ALLIED"]]->Begin(D3DXSPRITE_ALPHABLEND);
					Icons[Locations["INTERN_ALLIED"]]->SetTransform(&mat);
					Icons[Locations["INTERN_ALLIED"]]->Draw(NULL,NULL,NULL, 0xFFFFFFFF);
					Icons[Locations["INTERN_ALLIED"]]->End();
					break;
				}
				case pinfo::Nation::N_JAPAN:
				{
					Icons[Locations["INTERN_JAPAN"]]->Begin(D3DXSPRITE_ALPHABLEND);
					Icons[Locations["INTERN_JAPAN"]]->SetTransform(&mat);
					Icons[Locations["INTERN_JAPAN"]]->Draw(NULL,NULL,NULL, 0xFFFFFFFF);
					Icons[Locations["INTERN_JAPAN"]]->End();
					break;
				}
				case pinfo::Nation::N_SOVIET:
				{
					Icons[Locations["INTERN_SOVIET"]]->Begin(D3DXSPRITE_ALPHABLEND);
					Icons[Locations["INTERN_SOVIET"]]->SetTransform(&mat);
					Icons[Locations["INTERN_SOVIET"]]->Draw(NULL,NULL,NULL, 0xFFFFFFFF);
					Icons[Locations["INTERN_SOVIET"]]->End();
					break;
				}
				default:
				{
					Icons[0]->Begin(D3DXSPRITE_ALPHABLEND);
					Icons[0]->SetTransform(&mat);
					Icons[0]->Draw(NULL,NULL,NULL, 0xFFFFFFFF);
					Icons[0]->End();
					break;
				}
			}
			UNPROTECT;

			//draw player color
			D3DXMatrixTransformation2D( &mat, NULL, 0.0f, &D3DXVECTOR2(180.0f/64.0f,40.0f/64.0f), NULL, 0.0f, &D3DXVECTOR2(40.0f,(PlayerSpacing*findex)+15.0f) );			

			PROTECT;
			Icons[Locations["INTERN_BOX"]]->Begin(D3DXSPRITE_ALPHABLEND);
			Icons[Locations["INTERN_BOX"]]->SetTransform(&mat);
			Icons[Locations["INTERN_BOX"]]->Draw(NULL,NULL,NULL, *Player[player].Color);
			Icons[Locations["INTERN_BOX"]]->End();
			UNPROTECT;

			//draw name
			DirectXFont::Access(0)->Print(1.0,(PlayerSpacing*findex),-1,Player[player].Name.c_str());
			
			//draw money
			std::stringstream money;
			money.imbue(std::locale(""));
			money << ((*Player[player].Money < 1000) ? "{FFFF0000}" : "{FFFFFFFF}" ) << std::setw(13) << *Player[player].Money;
			DirectXFont::Access(0)->Print(40.0,(PlayerSpacing*findex)+15.0f,-1,money.str().c_str(),true);

			//draw power levels
			money.str( std::string() );
			money.clear();
			money << ((*Player[player].Usage > *Player[player].Power) ? "{FFFF0000}" : "{FF00FF00}" ) << std::setw(5) << *Player[player].Usage << " / " << std::setw(5) << *Player[player].Power;
			DirectXFont::Access(0)->Print(40.0,(PlayerSpacing*findex)+27.0f,-1,money.str().c_str(),true);
			
			//generate unit/building/stuff list
			PROTECT;
			for(int i = 0; ; ++i)
			{
				unsigned int TYPE = POINTER(unsigned int,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,Player[player].StuffPoolStart)+0x6C)+0x10)+0x1C)+(0x04+(0x04*i)))+0x3CC)+0x34);
				//unsigned int TYPE = POINTER(unsigned int,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,Player[player].StuffPoolStart) + (0x04 + (i*0x04))) + 0x100) + 0x210) + 0x34) + 0x08) + 0x3CC) + 0x34);
				//float Health = POINTER(float,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD, POINTER(DWORD,Player[SelectSlot].StuffPoolStart) + (0x04 + (i*0x04))) + 0x100) + 0x210) + 0x34) + 0x08) + 0x33C) + 0x04);
				//float PosX =   POINTER(float,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,				POINTER(DWORD,Player[SelectSlot].StuffPoolStart) + (0x04 + (i*0x04))) + 0x100) + 0x210) + 0x34) + 0x08) + 0x38);
				
				//uncomment if no question marks should be shown
				//if(StuffList[TYPE].first != NULL)
					++Stuff[StuffList[TYPE].first];
				//if(POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,Player[player].StuffPoolStart) + (0x04 + (i*0x04))) + 0x100) + 0x210) + 0x34) + 0x08) + 0x7C) == NULL)//pointer to next unit/building
				//	break;
				if(POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,Player[player].StuffPoolStart)+0x6C)+0x10)+0x1C)+(0x04+(0x04*i)))+0x7C) == NULL)//pointer to next unit/building
					break;
			}
			UNPROTECT;

			PROTECT;
			//draw unit/building/stuff list
			float column = 0.0f, row = 0.0f;
			for(auto it = Stuff.begin(); it != Stuff.end(); ++it)
			{
				if(it->second == 0)
					continue;
				D3DXMatrixTransformation2D( &mat, NULL, 0.0f, &D3DXVECTOR2(40.0f/64.0f,40.0f/64.0f), NULL, 0.0f, &D3DXVECTOR2(220.0f + (40.0f * column), (PlayerSpacing*findex) + (40.0f * row)) );			
	
				std::stringstream display("");
				display << std::setw(4) << it->second << "\0";
		
				Icons[it->first]->Begin(D3DXSPRITE_ALPHABLEND);
				Icons[it->first]->SetTransform(&mat);
				Icons[it->first]->Draw(NULL,NULL,NULL, 0xFFFFFFFF);
				Icons[it->first]->End();

				DirectXFont::Access(0)->Print((220.0f + (40.0f * (column+1.0f)))-DirectXFont::Access(0)->DrawLength("0000"),((PlayerSpacing*findex) + (40.0f * (row+1.0f)))-11.0f,0xFFFFFFFF,display.str().c_str());

				column += 1.0f;
				if(column >= (CurrRes.x/40.0f))
				{
					column = 0.0f;
					row += 1.0f;
				}
			}
			UNPROTECT;
		}
	}
	UNPROTECT;
}