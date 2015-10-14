/********************************************************
*                                                       *
*   Copyright (C) <CompanyName>. All Rights Reserved.   *
*                                                       *
********************************************************/

#pragma once

#include "stdafx.h"
#include "Utils.h"

#include <ctime>

namespace Utils
{
    std::string compile_time_str()
    {
        char s_month[5];
        int month, day, year, hour, minute, second;
        struct tm t = { 0 };
        static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        sscanf_s(__DATE__, "%s %d %d", &s_month, sizeof(s_month), &day, &year);
        sscanf_s(__TIME__, "%d:%d:%d", &hour, &minute, &second);

        month = (strstr(month_names, s_month) - month_names) / 3;

        t.tm_year = year - 1900;
        t.tm_mon = month;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = second;
        t.tm_isdst = -1;

        std::string timestamp;
        timestamp.resize(size_t(100));
        std::strftime(&timestamp[0], timestamp.size(), "%F %T", &t);

        return timestamp;
    }

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
