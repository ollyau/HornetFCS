/********************************************************
*                                                       *
*   Copyright (C) <CompanyName>. All Rights Reserved.   *
*                                                       *
********************************************************/

#pragma once

#include <codecvt>
#include <string>
#include <vector>

namespace Utils
{
    const unsigned long MAX_INI_READ_SIZE = 255;

    std::string compile_time_str();

    std::wstring ReadIni(std::string const& path, std::string const& section, std::string const& key);

    std::wstring s2ws(std::string const& str);

    std::string ws2s(std::wstring const& wstr);

    template <class TString>
    std::vector<TString> Split(TString const& s, TString const& d)
    {
        TString::size_type startPos = 0;
        TString::size_type endPos = s.find(d, startPos);

        std::vector<TString> result;

        while (endPos != TString::npos)
        {
            result.push_back(s.substr(startPos, endPos - startPos));
            startPos = endPos + d.size();

            endPos = s.find(d, startPos);
        }
        result.push_back(s.substr(startPos, s.size() - startPos));
        return result;
    }

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

    template <class TString>
    bool RemoveTrailingChar(TString& s, char c)
    {
        auto lastCharIdx = s.find_last_of(c);
        auto charExists = lastCharIdx != TString::npos;
        if (!charExists) { return false; }
        while (charExists && lastCharIdx + 1 == s.length())
        {
            s.pop_back();
            lastCharIdx = s.find_last_of(c);
        }
        return true;
    }

    template <class TString>
    bool RemoveLeadingChar(TString& s, char c)
    {
        auto firstCharIdx = s.find_first_of(c);
        auto charExists = firstCharIdx != TString::npos;
        if (!charExists) { return false; }
        while (charExists && firstCharIdx == 0)
        {
            s.erase(0, firstCharIdx + 1);
            firstCharIdx = s.find_first_of(c);
        }
        return true;
    }

} // namespace Utils
