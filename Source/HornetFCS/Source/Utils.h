/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#pragma once

#include <codecvt>
#include <string>
#include <vector>

namespace Utils
{

//-----------------------------------------------------------------------------

const unsigned long MAX_INI_READ_SIZE = 255;

//-----------------------------------------------------------------------------

std::string compile_time_str();

//-----------------------------------------------------------------------------

std::wstring ReadIni(std::string const& path, std::string const& section, std::string const& key);

//-----------------------------------------------------------------------------

std::wstring s2ws(std::string const& str);

//-----------------------------------------------------------------------------

std::string ws2s(std::wstring const& wstr);

//-----------------------------------------------------------------------------

template <class TString>
std::vector<double> SplitAndParse(TString const& s, TString const& d)
{
    TString::size_type startPos = 0;
    TString::size_type endPos = s.find(d, startPos);

    std::vector<double> result;

    while (endPos != TString::npos)
    {
        result.push_back(std::stod(s.substr(startPos, endPos - startPos)));
        startPos = endPos + d.size();

        endPos = s.find(d, startPos);
    }
    result.push_back(std::stod(s.substr(startPos, s.size() - startPos)));
    return result;
}

//-----------------------------------------------------------------------------

} // namespace Utils
