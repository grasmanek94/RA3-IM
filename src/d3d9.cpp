/*
* RA3INFO | 'Red Alert 3' By Gamer_Z/Grasmanek94 | http://gpb.googlecode.com/
*/
#include <main.h>
#define MAX_PLAYERS (6)

bool Displaying = false;

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

struct UnitsInfo
{
	//pointers to UnitsInfo structures, null when no structure available at that location
	DWORD * nodes[3];//0x00

	//data
	int unkown_1;		//0x0C
	unsigned int Type;	//0x10
	unsigned int Amount;//0x14
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
void OnGameLaunch();
extern D3DPRESENT_PARAMETERS* gl_myPresentationParams;

std::map<std::string,unsigned short> Locations;
std::map<DWORD*,UnitsInfo*> UnitList[MAX_PLAYERS];
std::map<unsigned int,std::string> UnitNames;
std::map<unsigned int,unsigned int> Units[MAX_PLAYERS];

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
		texture = NULL;
		sprite = NULL;
		_set = true;
	}
	bool Initialize(IDirect3DDevice9 *pDevice)
	{
		if(_set)
		{
			if(texture == NULL)
			{
				if(D3DXCreateTextureFromFileA( pDevice, toinit.c_str(), &texture ) != S_OK)
				{
					return false;
				}
			}
			if(texture != NULL)
			{
				if(sprite == NULL)
				{
					if(D3DXCreateSprite( pDevice, &sprite ) == S_OK)
					{
						return true;
					}
				}
			}
		}
		return false;
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

int ScreenY = 0;
int ScreenX = 0;
int MouseX  = 0;
int MouseY  = 0;

inline bool IsPlayerInGame()
{
	return (POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+0xD8) != NULL);
	//return (PlayerSlot != (-1));
}

namespace Drawer
{
	struct TextInfo
	{
		std::string text;
		float x,y;
		DWORD start,end;
		DWORD color;
		bool textcoloring;
		TextInfo(float _x, float _y, std::string _text, DWORD _start, DWORD _end, DWORD _color, bool _tc = false)
		{
			x = _x;
			y = _y;
			text.assign(_text.c_str());
			start = _start;
			end = _end;
			color = _color;
			textcoloring = _tc;
		}
	};

	std::vector<TextInfo> data;
	TextInfo DisplayNow(0.0f,10.0f,"",0,0,0xFFFFFFFF,true);

	void Add(float x, float y, std::string text, DWORD time, DWORD delay, DWORD color, bool TextColoring)
	{
		DWORD PUT = GetTickCount()+delay;
		data.push_back(TextInfo(x,y,text,PUT,PUT+time,color,TextColoring));
	}
	void AddQueue(std::string text, DWORD time = 1000)
	{
		DisplayNow.text.assign(text);
		DWORD PUT = GetTickCount();
		DisplayNow.start = PUT;
		DisplayNow.end = PUT+time;
	}
	void Display()
	{
		Displaying = false;
		DWORD now = GetTickCount();
		for(unsigned int i = 0, j = data.size(); i < j; ++i)
			if(data.at(i).end > now)
				data.erase(data.begin()+i);
		for(unsigned int i = 0, j = data.size(); i < j; ++i)
			if(data.at(i).start >= now)
			{
				Displaying = true;
				DirectXFont::Access(0)->Print(data.at(i).x,data.at(i).y,data.at(i).color,data.at(i).text.c_str(),data.at(i).textcoloring);
			}
			if(DisplayNow.end > now)
			{
				Displaying = true;
				//render->D3DBox(0.0,10.0f,DirectXFont::Access(0)->DrawLength(DisplayNow.text.c_str()),DirectXFont::Access(0)->DrawHeight(),0x77000000);
				DirectXFont::Access(0)->Print(0.0f,10.0f,0xFFFFFFFF,DisplayNow.text.c_str(),true);
			}
	}
};

namespace User
{
	struct Options
	{
		bool Cheat;
		bool Buildtest;
		bool Camera;
		unsigned int Transparency;
		Options() : 
			Cheat(true),Transparency(0x8F000000), Buildtest(true), Camera(true)
		{

		}
	};

	namespace Settings
	{
		Options Data;

		void CheckChange()
		{
			if(Keys(VK_APPS).Down)
			{
				if(Keys(VK_F9).ConsumePressed())
				{
					Data.Cheat ^= 1;
					if(!Data.Cheat)
						Drawer::AddQueue("Cheats {FFFF0000}Disabled");
					else
						Drawer::AddQueue("Cheats {FF00FF00}Enabled");
				}
				if(Keys(VK_F10).ConsumePressed())
				{
					Data.Buildtest ^= 1;
					if(!Data.Buildtest)
						Drawer::AddQueue("Buildtest {FFFF0000}Disabled");
					else
						Drawer::AddQueue("Buildtest {FF00FF00}Enabled");
				}
				if(Keys(VK_F12).ConsumePressed())
				{
					Data.Transparency += 0x10000000;
					if(Data.Transparency > 0xFF000000)
						Data.Transparency = 0xFF000000;
				}
				if(Keys(VK_F11).ConsumePressed())
				{
					Data.Transparency -= 0x10000000;
					if(Data.Transparency < 0x00000000)
						Data.Transparency = 0x00000000;
				}
			}//ConsumeDown(VK_APPS)
		}
	};
};



void GenerateUnitList(unsigned short slot)
{
	std::set<DWORD*> todo;
	unsigned int inserted = 1;
	while(inserted)
	{
		inserted = 0;
		for(auto it = UnitList[slot].begin(); it != UnitList[slot].end(); ++it)
		{
			for(short i = 0; i < 3; ++i)
			{
				if(it->second->nodes[i] != NULL)
				{
					if(UnitList[slot].find(&POINTER(DWORD,it->second->nodes[i])) == UnitList[slot].end())
					{
						todo.insert(it->second->nodes[i]);
						++inserted;
					}
				}
			}
		}
		while(!todo.empty())
		{
			UnitList[slot][*todo.begin()] = &POINTER(UnitsInfo,*todo.begin());
			todo.erase(todo.begin());
		}
	}
	for(auto it = UnitList[slot].begin(); it != UnitList[slot].end(); ++it)
		Units[slot][it->second->Type] = it->second->Amount;
}

void FrameTick(myIDirect3DDevice9 * device, HWND wnd)
{
	if(!GameInternsInited)
		OnGameLaunch();
	PROTECT;

	KeyManagerRun();//key press/release detection
	if(Keys(VK_MENU).Down && Keys(VK_F4).Pressed)
		::ExitProcess(0);

	MouseX = POINTER(int,POINTER(DWORD,0x00CDAEFC)+0x3C);
	MouseY = POINTER(int,POINTER(DWORD,0x00CDAEFC)+0x40);
	ScreenX = POINTER(int, 0x00400000+0x008E9EBC);
	ScreenY = POINTER(int, 0x00400000+0x008E9EC0);

	User::Settings::CheckChange();
	Drawer::Display();

	static bool got_session = false;
	static float OldCamerainfo[5] = {0.0f,0.0f, 0.0f, 0.0f, 0.0f};
	static bool ChangedCameHeight = false;
	static bool ChangedCamYawRollPitch = false;

	if(!User::Settings::Data.Cheat || !IsPlayerInGame())
	{
		if(!IsPlayerInGame())
		{
			OldCamerainfo[0] = 0.0f;
			OldCamerainfo[1] = 0.0f;
			OldCamerainfo[2] = 0.0f;
			OldCamerainfo[3] = 0.0f;
			OldCamerainfo[4] = 0.0f;
			ChangedCameHeight = false;
			ChangedCamYawRollPitch = false;
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
					UnitList[i].clear();
					Units[i].clear();
				}
			}
			if(!Displaying)
			{
				DirectXFont::Access(0)->Print(0.0,0.0,0xFFFF0000, "RA3{FFFFFF00}INFO {FF00FFFF}MOD {FFFFFFFF}By Gamer_Z a.k.a Grasmanek94",true);
				DirectXFont::Access(0)->Print(0.0,11.0,0xFFFFFF00,"Website: {FF00FFFF}http://gpb.googlecode.com/",true);
			}
		}
		return;
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
					//MessageBox(NULL,GameSessionInfo,"char",0);
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
								Player[slot].StuffPoolStart = &POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,0x00400000+0x008E98EC)+CurrentSlot)+0x20C)+0xC)+0x22C);
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
	if(User::Settings::Data.Camera)
	{
		if(Keys(VK_MENU).Down)
		{
			if(Keys(VK_CONTROL).Down)
			{
				if(Keys('R').Pressed)
				{
					if(ChangedCameHeight)
					{
						float& CamHeightMax = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x2704);
						float& CamHeightMin = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x2700);
						float& CamHeight = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x44);
						CamHeightMax = OldCamerainfo[0];
						CamHeightMin = OldCamerainfo[1];
						CamHeight = CamHeightMax;
					}
					if(ChangedCamYawRollPitch)
					{
						float& CamPitch = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x2C);
						float& CamRoll = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x30);
						CamPitch = OldCamerainfo[2];
						CamRoll = OldCamerainfo[3];
					}
				}
				if(Keys(VK_OEM_6).Pressed)//]
				{
					float& ViewDistance = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x34);
					ViewDistance += 1.0f;
				}
				if(Keys(VK_OEM_4).Pressed)//[
				{
					float& ViewDistance = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x34);
					ViewDistance -= 1.0f;
					if(ViewDistance <= 1.0f)
						ViewDistance = 1.0f;
				}
			}
			else
			{
				if(Keys(VK_OEM_6).Pressed)//]
				{
					float& CamHeightMax = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x2704);
					float& CamHeightMin = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x2700);
					float& CamHeight = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x44);
					if(!ChangedCameHeight)
					{
						OldCamerainfo[0] = CamHeightMax;
						OldCamerainfo[1] = CamHeightMin;
						ChangedCameHeight = true;
					}
					CamHeightMax += 200.0f;
					CamHeight = CamHeightMax;
				}
				if(Keys(VK_OEM_4).Pressed)//[
				{
					float& CamHeightMax = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x2704);
					float& CamHeightMin = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x2700);
					float& CamHeight = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x44);
					if(!ChangedCameHeight)
					{
						OldCamerainfo[0] = CamHeightMax;
						OldCamerainfo[1] = CamHeightMin;
						ChangedCameHeight = true;
					}
					CamHeightMax -= 200.0f;
					if(CamHeightMax < 1.0f)
						CamHeightMax = 1.0f;
					if(CamHeightMin > CamHeightMax)
						CamHeightMin = CamHeightMax;
					CamHeight = CamHeightMax;
				}
			}
		}
		else
		{
			if(Keys(VK_CONTROL).Down)
			{
				static float SavedMouse[2] = {0.00f,0.0f};
				float& CamYaw = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x28);
				if(Keys(VK_SPACE).Down)
				{
					float& CamPitch = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x2C);
					float& CamRoll = POINTER(float,POINTER(DWORD,0x00CDB7B4)+0x30);
					if(!ChangedCamYawRollPitch)
					{
						OldCamerainfo[2] = CamPitch;
						OldCamerainfo[3] = CamRoll;
						ChangedCamYawRollPitch = true;
					}
					float MouseChange[2] = {SavedMouse[0]-(float)MouseX,SavedMouse[1]-(float)MouseY};
					CamRoll = OldCamerainfo[3] + (MouseChange[0]*0.006);
					CamPitch = OldCamerainfo[2] + (MouseChange[1]*0.008);
					CamYaw = OldCamerainfo[4];
				}
				else
				{
					OldCamerainfo[4] = CamYaw;
					SavedMouse[0] = MouseX;
					SavedMouse[1] = MouseY;
				}
			}
		}
	}
	if(!Displaying)
	{
		float CurrDrawHeight = 10.0f;
		float StartWidth = 0.0f;
		short SelectSlot = -1;
		for(int i = 0; i < 6; ++i)
		{
			if(!Player[i].in)	
				break;

			std::stringstream display("");
			switch(Player[i].Side)
			{
				case pinfo::Nation::N_ALLIES:{display << "A ";break;}
				case pinfo::Nation::N_JAPAN: {display << "J ";break;}
				case pinfo::Nation::N_SOVIET:{display << "S ";break;}
				default:					 {display << "U ";break;}
			}
			if(!Player[i].GotUnits)
			{
				if(POINTER(DWORD,*Player[i].unit_start_node+0x10) > 256)
				{
					Player[i].GotUnits = true;
					UnitList[i][(&POINTER(UnitsInfo,*Player[i].unit_start_node))->nodes[2]] = &POINTER(UnitsInfo,*Player[i].unit_start_node);
					GenerateUnitList(i);
				}
			}
			else
				GenerateUnitList(i);
			StartWidth = DirectXFont::Access(0)->DrawLength("0 000000000000000000000000 0000000 0000/0000")+10.0f;
			render->D3DBox(5.0f,CurrDrawHeight-1.0f,DirectXFont::Access(0)->DrawLength("0 000000000000000000000000 0000000 0000/0000")+10.0f,15.0f,(User::Settings::Data.Transparency + (*Player[i].Color & 0x00FFFFFF)));
			if(MouseX >= 5 && MouseX <= (int)(DirectXFont::Access(0)->DrawLength("0 000000000000000000000000 0000000 0000/0000")+10.0f) && MouseY >= (int)(CurrDrawHeight-1.0f) && MouseY <= (int)((CurrDrawHeight-1.0f)+15.0f))
				SelectSlot = i;
			display << std::setw(24) << Player[i].Name.c_str() << " " << std::setw(7) << *Player[i].Money << "{FF" << ((*Player[i].Usage > *Player[i].Power) ? ("FF00") : ((*Player[i].Usage >= (*Player[i].Power-25)) ? ("FF7F") : ("00FF"))) << "00} " << std::setw(4) << *Player[i].Usage << "/" << std::setw(4) << *Player[i].Power << "\0";
			DirectXFont::Access(0)->Print(10.0,CurrDrawHeight,0xFFFFFFFF,display.str().c_str(),true);
			CurrDrawHeight += 15.0f;
		}

		if(SelectSlot != -1)
		{
			//DirectXFont::Access(0)->Print(MouseX,MouseY+5,0xFFFFFFFF,string_format("Current Selected player: %s",Player[SelectSlot].Name.c_str()).c_str(),true);
			D3DXVECTOR2 Scaling( (1.0f/(800.0f/(float)gl_myPresentationParams->BackBufferWidth))*(48.0f/96.0f),(1.0f/(600.0f/(float)gl_myPresentationParams->BackBufferHeight))*(48.0f/96.0f) );
			D3DXVECTOR2 Adders ( 52.0f * (48.0f/96.0f) / Scaling.x, 52.0f * (48.0f/96.0f) / Scaling.y);
			D3DXVECTOR2 DrawMTX (  ((float)gl_myPresentationParams->BackBufferWidth/1024.0f) / Scaling.x, ((float)gl_myPresentationParams->BackBufferHeight/768.0f) / Scaling.y);
			D3DXMATRIX mat;
			D3DXMatrixTransformation2D( &mat, &D3DXVECTOR2(0.0f,0.0f), 0.0f, &Scaling, NULL, 0.0f, NULL );

			if(User::Settings::Data.Buildtest)
			{
				if(Keys(VK_MENU).Down)
				{
					/*if(Keys(VK_CONTROL).Down)
					{
						std::map<unsigned short,unsigned int> Stuff;
						for(int i = 0; i < *Player[SelectSlot].TotalAmountOfStuff; ++i)
						{
							unsigned int TYPE = POINTER(unsigned int,POINTER(DWORD,POINTER(DWORD,POINTER(DWORD,Player[SelectSlot].StuffPoolStart) + (0x04 + (i*0x04))) + 0x33C) - 0x0C);
							++Stuff[StuffList[TYPE].first];
						}
						float column = 0.0f, row = 0.0f;
						for(auto it = Stuff.begin(); it != Stuff.end(); ++it)
						{
							std::stringstream display("");
							display << std::setw(4) << it->second << "\0";

							Icons[it->first]->Begin(D3DXSPRITE_ALPHABLEND);
							Icons[it->first]->SetTransform(&mat);
							Icons[it->first]->Draw(NULL,NULL,&D3DXVECTOR3( (Adders.x * column + DirectXFont::Access(0)->DrawLength("0 000000000000000000000000 0000000 0000/0000")+17.5f)*DrawMTX.x , (Adders.y * row) * DrawMTX.y, 0.0f), 0xFFFFFFFF);
							Icons[it->first]->End();

							DirectXFont::Access(0)->Print(((Adders.x * (column+1.0f)) + DirectXFont::Access(0)->DrawLength("0 000000000000000000000000 0000000 0000/0000")+17.5f)-(DirectXFont::Access(0)->DrawLength("0000")+2.5f),(Adders.y * (row+1.0f)) - 15.0f,0xCCFFFFFF,display.str().c_str());

							column += 1.0f;
							if(column == 8.0f)
							{
								column = 0.0f;
								row += 1.0f;
							}
						}
					}
					else*/
					{
						//CurrDrawHeight = 10.0f;
						//StartWidth += 10.0f;
						float column = 0.0f, row = 0.0f;
						for(auto it = Units[SelectSlot].begin(); it != Units[SelectSlot].end(); ++it)
						{
							if(it->second > 0)
							{
								std::stringstream display("");
								display << std::setw(4) << it->second << "\0";
								//render->D3DBox(StartWidth,CurrDrawHeight-1.0f,DirectXFont::Access(0)->DrawLength(display.str().c_str())+10.0f,15.0f,0xAA000000);
								//DirectXFont::Access(0)->Print(StartWidth+5.0f,CurrDrawHeight,0xFFFFFFFF,display.str().c_str(),true);
								//CurrDrawHeight += 15.0f;
							
								Icons[Locations[UnitNames[it->first]]]->Begin(D3DXSPRITE_ALPHABLEND);
								Icons[Locations[UnitNames[it->first]]]->SetTransform(&mat);
								Icons[Locations[UnitNames[it->first]]]->Draw(NULL,NULL,&D3DXVECTOR3( (Adders.x * column + DirectXFont::Access(0)->DrawLength("0 000000000000000000000000 0000000 0000/0000")+17.5f)*DrawMTX.x , (Adders.y * row) * DrawMTX.y, 0.0f), 0xFFFFFFFF);
								Icons[Locations[UnitNames[it->first]]]->End();

								DirectXFont::Access(0)->Print(((Adders.x * (column+1.0f)) + DirectXFont::Access(0)->DrawLength("0 000000000000000000000000 0000000 0000/0000")+17.5f)-(DirectXFont::Access(0)->DrawLength("0000")+2.5f),(Adders.y * (row+1.0f)) - 15.0f,0xCCFFFFFF,display.str().c_str());

								column += 1.0f;
								if(column == 8.0f)
								{
									column = 0.0f;
									row += 1.0f;
								}
							}
						}
					}
				}
			}
		}
	}
	UNPROTECT;
}


void InitializeDX(myIDirect3DDevice9 * device)
{
	DirectXFont::Add("Lucida Console",10.0f,FW_BOLD);
	DirectXFont::InitializeAll();
}

void UninitializeDX(myIDirect3DDevice9 * device)
{
	DirectXFont::InvalidateAll();
}

void texturesInitResources ( IDirect3DDevice9 *pDevice, bool init = true)
{
	static bool Created = false;
	if(init)
	{
		for(auto it = Locations.begin(); it != Locations.end(); ++it)
		{
			if(!Created)
				Icons.emplace(std::pair<unsigned short,Ra3Icon*>(it->second,new Ra3Icon(it->second)));
			Icons[it->second]->Initialize(pDevice);
		}
		Created = true;
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

	//Japan
	UnitNames[472840039]  = "Japan MCV";
	UnitNames[3333683370] = "Generator Core";
	UnitNames[1475377722] = "Dojo Core";
	UnitNames[4283075424] = "Refinery Core";
	UnitNames[2462967055] = "Japan Ore Collector";
	UnitNames[317183908]  = "Burst Drone";
	UnitNames[263203925]  = "Imperial Warrior";
	UnitNames[2097498589] = "Tankbuster";
	UnitNames[551280588]  = "Japan Engineer";
	UnitNames[2687381971] = "Mecha Bay Core";
	UnitNames[2027152172] = "Mainframe Core";
	UnitNames[4031077785] = "Docks Core";
	UnitNames[2808668694] = "Shinobi";
	UnitNames[395436146]  = "Sudden Transport";
	UnitNames[524905323]  = "Defender Core";
	UnitNames[1703323040] = "Yuriko Omega";
	UnitNames[3625717778] = "Rocket Angle";
	UnitNames[3281546964] = "Mecha Tengu";
	UnitNames[3468155423] = "Tsunami Tank";
	UnitNames[1854693375] = "Striker-VX";
	UnitNames[1502645858] = "King Oni";
	UnitNames[2427985212] = "Wave-Force Artillery";
	UnitNames[3470418429] = "Yar Mini-Sub";
	UnitNames[2973949638] = "Sea-Wing";
	UnitNames[439134635]  = "Naginata Cruiser";
	UnitNames[4234031892] = "Shogun Battleship";
	UnitNames[605223810]  = "Tower Core";
	UnitNames[1607020577] = "Nanoswarm Core";
	UnitNames[1545342735] = "Decimator Core";

	//Allies
	UnitNames[685397838]  = "Allied MCV";
	UnitNames[1015200016] = "Apollo Fighter";
	UnitNames[3075441480] = "Vindicator";
	UnitNames[706309745]  = "Prospector";
	UnitNames[1352487169] = "Cryocopter";
	UnitNames[3724290270] = "Attack Dog";
	UnitNames[329038999]  = "Peacekeeper";
	UnitNames[2211817579] = "Century Bomber";
	UnitNames[2623355832] = "Javelin Soldier";
	UnitNames[3790149531] = "Allied Engineer";
	UnitNames[1252381973] = "Spy";
	UnitNames[1407249170] = "Tanya";
	UnitNames[1080603607] = "Rapicide ACV";
	UnitNames[3137747290] = "Multigunner IFV";
	UnitNames[129570087]  = "Guardian Tank";
	UnitNames[3566131256] = "Athena Cannon";
	UnitNames[1388308933] = "Mirage Tank";
	UnitNames[2328510325] = "Dolphin";
	UnitNames[773921433]  = "Hydrofoil";
	UnitNames[1524970748] = "Assault Destroyer";
	UnitNames[158358912]  = "Aircraft Carrier";

	//Soviet
	UnitNames[4141531812] = "Soviet Ore Collector";
	UnitNames[855797665]  = "War Bear";
	UnitNames[430641583]  = "Conscript";
	UnitNames[741706849]  = "Flak Trooper";
	UnitNames[2693442515] = "Soviet Engineer";
	UnitNames[1041335901] = "Tesla Trooper";
	UnitNames[391087951]  = "Natasha";
	UnitNames[3144182497] = "Sputnik";
	UnitNames[3611415335] = "Terror Drone";
	UnitNames[1626387689] = "Sickle";
	UnitNames[1378601825] = "Bullfrog";
	UnitNames[2494781707] = "Hammer Tank";
	UnitNames[2182316510] = "V4 Rocket Launcher";
	UnitNames[2655336508] = "Apocalypse Tank";
	UnitNames[2940997029] = "Soviet MCV";
	UnitNames[2136264401] = "MiG Fighter";
	UnitNames[1085058413] = "Twinblade";
	UnitNames[4289204654] = "Kirov Airship";
	UnitNames[2952146552] = "Stingray";
	UnitNames[2366257226] = "Akula Sub";
	UnitNames[3169800633] = "Dreadnought";

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
	Locations["Rapicide ACV"] = 0x16;
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
	Locations["Yar Mini-Sub"] = 0x53;
	Locations["Yuriko Omega"] = 0x54;
	Locations["Sea-Wing"] = 0x55;
	Locations["Naginata Cruiser"] = 0x56;
	Locations["Wave-Force Artillery"] = 0x57;
	Locations["Japan MCV"] = 0x58;
	Locations["Shogun Battleship"] = 0x59;

	Locations["Tower Core"] = 0x91;
	Locations["Defender Core"] = 0x92;
	Locations["Decimator Core"] = 0x93;
	Locations["Nanoswarm Core"] = 0x94;
	Locations["Mainframe Core"] = 0x95;
	Locations["Refinery Core"] = 0x96;
	Locations["Dojo Core"] = 0x97;
	Locations["Docks Core"] = 0x98;
	Locations["Mecha Bay Core"] = 0x99;
	Locations["Generator Core"] = 0x9A;

	//Soviet
	Locations["Hammer Tank"] = 0x4A;
	Locations["Terror Drone"] = 0x3A;
	Locations["Soviet MCV"] = 0x5A;
	Locations["Reactor"] = 0x60;
	Locations["Barracks"] = 0x61;
	Locations["Soviet Ore Refinery"] = 0x62;
	Locations["Soviet Wall"] = 0x63;
	Locations["Sentry Gun"] = 0x64;
	Locations["War Bear"] = 0x65;
	Locations["Flak Cannon"] = 0x66;
	Locations["Conscript"] = 0x67;
	Locations["Flak Trooper"] = 0x68;
	Locations["Soviet Ore Collector"] = 0x69;
	Locations["Sputnik"] = 0x6A;
	Locations["War Factory"] = 0x70;
	Locations["Naval Yard"] = 0x71;
	Locations["Airfield"] = 0x72;
	Locations["Tesla Coil"] = 0x73;
	Locations["Iron Curtain"] = 0x74;
	Locations["Soviet Engineer"] = 0x75;
	Locations["Vacuum Imploder"] = 0x76;
	Locations["Tesla Trooper"] = 0x77;
	Locations["Natasha"] = 0x78;
	Locations["Sickle"] = 0x79;
	Locations["Bullfrog"] = 0x7A;
	Locations["Super-Reactor"] = 0x80;
	Locations["Battle Lab"] = 0x81;
	Locations["Crusher Crane"] = 0x82;
	Locations["Stringray"] = 0x83;
	Locations["Akula Sub"] = 0x84;
	Locations["Twinblade"] = 0x85;
	Locations["Dreadnought"] = 0x86;
	Locations["MiG Fighter"] = 0x87;
	Locations["Kirov Airship"] = 0x88;
	Locations["V4 Rocket Launcher"] = 0x89;
	Locations["Apocalypse Tank"] = 0x8A;

	//
	StuffList[0] = std::pair<unsigned short,StuffInfo>(0x00,StuffInfo());
	StuffList[474133264] = std::pair<unsigned short,StuffInfo>(0x01,StuffInfo());
	StuffList[473791240] = std::pair<unsigned short,StuffInfo>(0x02,StuffInfo());
	StuffList[474094136] = std::pair<unsigned short,StuffInfo>(0x03,StuffInfo());
	StuffList[473763536] = std::pair<unsigned short,StuffInfo>(0x04,StuffInfo());
	StuffList[474008248] = std::pair<unsigned short,StuffInfo>(0x05,StuffInfo());
	StuffList[473867608] = std::pair<unsigned short,StuffInfo>(0x06,StuffInfo());
	StuffList[474174952] = std::pair<unsigned short,StuffInfo>(0x07,StuffInfo());
	StuffList[474376408] = std::pair<unsigned short,StuffInfo>(0x08,StuffInfo());
	StuffList[474079936] = std::pair<unsigned short,StuffInfo>(0x09,StuffInfo());
	StuffList[474270024] = std::pair<unsigned short,StuffInfo>(0x0A,StuffInfo());
	StuffList[473655464] = std::pair<unsigned short,StuffInfo>(0x10,StuffInfo());
	StuffList[473848096] = std::pair<unsigned short,StuffInfo>(0x11,StuffInfo());
	StuffList[474429040] = std::pair<unsigned short,StuffInfo>(0x12,StuffInfo());
	StuffList[473741600] = std::pair<unsigned short,StuffInfo>(0x13,StuffInfo());
	StuffList[474339632] = std::pair<unsigned short,StuffInfo>(0x14,StuffInfo());
	StuffList[473960080] = std::pair<unsigned short,StuffInfo>(0x15,StuffInfo());
	StuffList[473933072] = std::pair<unsigned short,StuffInfo>(0x16,StuffInfo());
	StuffList[474228888] = std::pair<unsigned short,StuffInfo>(0x17,StuffInfo());
	StuffList[473989256] = std::pair<unsigned short,StuffInfo>(0x18,StuffInfo());
	StuffList[474287808] = std::pair<unsigned short,StuffInfo>(0x19,StuffInfo());
	StuffList[474249728] = std::pair<unsigned short,StuffInfo>(0x1A,StuffInfo());
	StuffList[473689368] = std::pair<unsigned short,StuffInfo>(0x20,StuffInfo());
	StuffList[473677120] = std::pair<unsigned short,StuffInfo>(0x21,StuffInfo());
	StuffList[474110528] = std::pair<unsigned short,StuffInfo>(0x22,StuffInfo());
	StuffList[473721904] = std::pair<unsigned short,StuffInfo>(0x23,StuffInfo());
	StuffList[474196360] = std::pair<unsigned short,StuffInfo>(0x24,StuffInfo());
	StuffList[474048416] = std::pair<unsigned short,StuffInfo>(0x25,StuffInfo());
	StuffList[474389904] = std::pair<unsigned short,StuffInfo>(0x26,StuffInfo());
	StuffList[474152104] = std::pair<unsigned short,StuffInfo>(0x27,StuffInfo());
	StuffList[474413424] = std::pair<unsigned short,StuffInfo>(0x28,StuffInfo());
	StuffList[473707000] = std::pair<unsigned short,StuffInfo>(0x29,StuffInfo());
	StuffList[474306392] = std::pair<unsigned short,StuffInfo>(0x2A,StuffInfo());
	StuffList[470566496] = std::pair<unsigned short,StuffInfo>(0x30,StuffInfo());
	StuffList[470862936] = std::pair<unsigned short,StuffInfo>(0x31,StuffInfo());
	StuffList[470495744] = std::pair<unsigned short,StuffInfo>(0x32,StuffInfo());
	StuffList[472994640] = std::pair<unsigned short,StuffInfo>(0x33,StuffInfo());
	StuffList[473049144] = std::pair<unsigned short,StuffInfo>(0x34,StuffInfo());
	StuffList[473470424] = std::pair<unsigned short,StuffInfo>(0x35,StuffInfo());
	StuffList[473571064] = std::pair<unsigned short,StuffInfo>(0x36,StuffInfo());
	StuffList[473123920] = std::pair<unsigned short,StuffInfo>(0x37,StuffInfo());
	StuffList[473218696] = std::pair<unsigned short,StuffInfo>(0x38,StuffInfo());
	StuffList[473537480] = std::pair<unsigned short,StuffInfo>(0x39,StuffInfo());
	StuffList[472168696] = std::pair<unsigned short,StuffInfo>(0x3A,StuffInfo());
	StuffList[470286576] = std::pair<unsigned short,StuffInfo>(0x40,StuffInfo());
	StuffList[470621680] = std::pair<unsigned short,StuffInfo>(0x41,StuffInfo());
	StuffList[470361816] = std::pair<unsigned short,StuffInfo>(0x42,StuffInfo());
	StuffList[470828944] = std::pair<unsigned short,StuffInfo>(0x43,StuffInfo());
	StuffList[473269424] = std::pair<unsigned short,StuffInfo>(0x44,StuffInfo());
	StuffList[473449464] = std::pair<unsigned short,StuffInfo>(0x45,StuffInfo());
	StuffList[473236024] = std::pair<unsigned short,StuffInfo>(0x46,StuffInfo());
	StuffList[473415960] = std::pair<unsigned short,StuffInfo>(0x47,StuffInfo());
	StuffList[473612464] = std::pair<unsigned short,StuffInfo>(0x48,StuffInfo());
	StuffList[473375064] = std::pair<unsigned short,StuffInfo>(0x49,StuffInfo());
	StuffList[472664784] = std::pair<unsigned short,StuffInfo>(0x4A,StuffInfo());
	StuffList[470442688] = std::pair<unsigned short,StuffInfo>(0x50,StuffInfo());
	StuffList[470401416] = std::pair<unsigned short,StuffInfo>(0x51,StuffInfo());
	StuffList[470765808] = std::pair<unsigned short,StuffInfo>(0x52,StuffInfo());
	StuffList[473092664] = std::pair<unsigned short,StuffInfo>(0x53,StuffInfo());
	StuffList[473302536] = std::pair<unsigned short,StuffInfo>(0x54,StuffInfo());
	StuffList[473634816] = std::pair<unsigned short,StuffInfo>(0x55,StuffInfo());
	StuffList[473435176] = std::pair<unsigned short,StuffInfo>(0x56,StuffInfo());
	StuffList[473503800] = std::pair<unsigned short,StuffInfo>(0x57,StuffInfo());
	StuffList[473201944] = std::pair<unsigned short,StuffInfo>(0x58,StuffInfo());
	StuffList[473521024] = std::pair<unsigned short,StuffInfo>(0x59,StuffInfo());
	StuffList[472380232] = std::pair<unsigned short,StuffInfo>(0x5A,StuffInfo());
	StuffList[472270672] = std::pair<unsigned short,StuffInfo>(0x60,StuffInfo());
	StuffList[472607432] = std::pair<unsigned short,StuffInfo>(0x61,StuffInfo());
	StuffList[472225672] = std::pair<unsigned short,StuffInfo>(0x62,StuffInfo());
	StuffList[472063720] = std::pair<unsigned short,StuffInfo>(0x63,StuffInfo());
	StuffList[472554520] = std::pair<unsigned short,StuffInfo>(0x64,StuffInfo());
	StuffList[472198584] = std::pair<unsigned short,StuffInfo>(0x65,StuffInfo());
	StuffList[472573168] = std::pair<unsigned short,StuffInfo>(0x66,StuffInfo());
	StuffList[472840912] = std::pair<unsigned short,StuffInfo>(0x67,StuffInfo());
	StuffList[472702600] = std::pair<unsigned short,StuffInfo>(0x68,StuffInfo());
	StuffList[472332312] = std::pair<unsigned short,StuffInfo>(0x69,StuffInfo());
	StuffList[472094200] = std::pair<unsigned short,StuffInfo>(0x6A,StuffInfo());
	StuffList[472043032] = std::pair<unsigned short,StuffInfo>(0x70,StuffInfo());
	StuffList[472290264] = std::pair<unsigned short,StuffInfo>(0x71,StuffInfo());
	StuffList[472928280] = std::pair<unsigned short,StuffInfo>(0x72,StuffInfo());
	StuffList[472589560] = std::pair<unsigned short,StuffInfo>(0x73,StuffInfo());
	StuffList[472149080] = std::pair<unsigned short,StuffInfo>(0x74,StuffInfo());
	StuffList[472448696] = std::pair<unsigned short,StuffInfo>(0x75,StuffInfo());
	StuffList[472126184] = std::pair<unsigned short,StuffInfo>(0x76,StuffInfo());
	StuffList[472403264] = std::pair<unsigned short,StuffInfo>(0x77,StuffInfo());
	StuffList[472506696] = std::pair<unsigned short,StuffInfo>(0x78,StuffInfo());
	StuffList[472810752] = std::pair<unsigned short,StuffInfo>(0x79,StuffInfo());
	StuffList[472912848] = std::pair<unsigned short,StuffInfo>(0x7A,StuffInfo());
	StuffList[472252096] = std::pair<unsigned short,StuffInfo>(0x80,StuffInfo());
	StuffList[472076288] = std::pair<unsigned short,StuffInfo>(0x81,StuffInfo());
	StuffList[472480384] = std::pair<unsigned short,StuffInfo>(0x82,StuffInfo());
	StuffList[472789104] = std::pair<unsigned short,StuffInfo>(0x83,StuffInfo());
	StuffList[472772384] = std::pair<unsigned short,StuffInfo>(0x84,StuffInfo());
	StuffList[472883224] = std::pair<unsigned short,StuffInfo>(0x85,StuffInfo());
	StuffList[472757352] = std::pair<unsigned short,StuffInfo>(0x86,StuffInfo());
	StuffList[472429160] = std::pair<unsigned short,StuffInfo>(0x87,StuffInfo());
	StuffList[472541048] = std::pair<unsigned short,StuffInfo>(0x88,StuffInfo());
	StuffList[472742288] = std::pair<unsigned short,StuffInfo>(0x89,StuffInfo());
	StuffList[472626376] = std::pair<unsigned short,StuffInfo>(0x8A,StuffInfo());
	StuffList[473814280] = std::pair<unsigned short,StuffInfo>(0x90,StuffInfo());
	StuffList[473341688] = std::pair<unsigned short,StuffInfo>(0x91,StuffInfo());
	StuffList[473327232] = std::pair<unsigned short,StuffInfo>(0x92,StuffInfo());
	StuffList[473034240] = std::pair<unsigned short,StuffInfo>(0x93,StuffInfo());
	StuffList[473019888] = std::pair<unsigned short,StuffInfo>(0x94,StuffInfo());
	StuffList[473005552] = std::pair<unsigned short,StuffInfo>(0x95,StuffInfo());
	StuffList[473063944] = std::pair<unsigned short,StuffInfo>(0x96,StuffInfo());
	StuffList[473356168] = std::pair<unsigned short,StuffInfo>(0x97,StuffInfo());
	StuffList[473107424] = std::pair<unsigned short,StuffInfo>(0x98,StuffInfo());
	StuffList[472980168] = std::pair<unsigned short,StuffInfo>(0x99,StuffInfo());
	StuffList[473078272] = std::pair<unsigned short,StuffInfo>(0x9A,StuffInfo());


	Drawer::AddQueue(" ",0);
}