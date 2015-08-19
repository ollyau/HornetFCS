/********************************************************
*                                                       *
*   Copyright (C) <CompanyName>. All Rights Reserved.   *
*                                                       *
********************************************************/

#pragma once

#include "stdafx.h"
#include "Utils.h"

namespace Utils
{
    std::wstring ReadIni(std::string const& path, std::string const& section, std::string const& key)
    {
        wchar_t buf[MAX_INI_READ_SIZE];
        GetPrivateProfileString(Utils::s2ws(section).c_str(), Utils::s2ws(key).c_str(), NULL, buf, sizeof(buf), Utils::s2ws(path).c_str());
        return std::wstring(buf);
    }

    std::wstring s2ws(std::string const& str)
    {
        typedef std::codecvt_utf8<wchar_t> convert_typeX;
        std::wstring_convert<convert_typeX, wchar_t> converterX;
        return converterX.from_bytes(str);
    }

    std::string ws2s(std::wstring const& wstr)
    {
        typedef std::codecvt_utf8<wchar_t> convert_typeX;
        std::wstring_convert<convert_typeX, wchar_t> converterX;
        return converterX.to_bytes(wstr);
    }
}
