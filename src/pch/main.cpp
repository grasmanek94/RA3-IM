#include <main.h>

BOOL APIENTRY DllMain ( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
	process_init(  hModule,   ul_reason_for_call,  lpReserved);
	return true;
}

namespace DirectXFont
{
	std::map<std::pair<std::string,std::pair<int,DWORD>>,CD3DFont> fonts;
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
		fonts.emplace(std::make_pair(fontname,std::make_pair(size,flags)),CD3DFont(fontname.c_str(),size,flags));
		font_id[++FontCounter] = std::make_pair(fontname,std::make_pair(size,flags));
		return FontCounter;
	}
	bool Initialize(unsigned int ID)
	{
		if(ID < 0)
			return false;
		if(fonts.find(font_id[ID]) == fonts.end())
			return false;
		return fonts.at(font_id[ID]).Initialize( gl_pmyIDirect3DDevice9 ) == S_OK;
	}
	bool Remove(unsigned int ID)
	{
		if(ID < 0)
			return false;
		if(fonts.find(font_id[ID]) == fonts.end())
			return false;
		fonts.at(font_id[ID]).Invalidate();
		fonts.erase(font_id[ID]);
		return true;
	}
	void InitializeAll()
	{
		for(auto it = fonts.begin(); it != fonts.end(); ++it)
			it->second.Initialize( gl_pmyIDirect3DDevice9 );
	}
	void InvalidateAll()
	{
		for(auto it = fonts.begin(); it != fonts.end(); ++it)
			it->second.Invalidate();
	}
	CD3DFont * Access(int ID)
	{
		if(ID < 0)
			return 0;
		if(fonts.find(font_id[ID]) == fonts.end())
			return 0;
		return &fonts.at(font_id[ID]);
	}
};

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

KeyManager Keys[256];

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