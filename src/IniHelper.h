#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>

class IniHelper {
public:
    // 在指定Key后面插入新的键值对
    static bool InsertValue(const std::wstring& Path, const std::wstring& Section, const std::wstring& AfterKey, const std::wstring& Key, const std::wstring& Value) {
        std::wifstream iniFile(Path);
        std::wstring line;
        bool sectionFound = false;
        bool afterKeyFound = false;
        bool keyInserted = false;
        std::wofstream outFile(Path + L".tmp");

        if (!outFile.is_open()) {
            return false;
        }

        // 如果原文件存在且可以打开，则读取内容
        if (iniFile.is_open()) {
            while (std::getline(iniFile, line)) {
                if (line.empty() || std::all_of(line.begin(), line.end(), ::iswspace)) {
                    outFile << line << std::endl;
                    continue;
                }

                if (line.find(L"[" + Section + L"]") != std::wstring::npos) {
                    sectionFound = true;
                    outFile << line << std::endl;
                    continue;
                }

                if (!sectionFound) {
                    outFile << line << std::endl;
                    continue;
                }

                if (line.front() == L'[') {
                    if (!keyInserted && afterKeyFound) {
                        outFile << Key << L"=" << Value << std::endl;
                        keyInserted = true;
                    }
                    outFile << line << std::endl;
                    continue;
                }

                size_t equalPos = line.find(L"=");
                if (equalPos != std::wstring::npos) {
                    std::wstring currentKey = line.substr(0, equalPos);
                    outFile << line << std::endl;
                    
                    if (currentKey == AfterKey && !keyInserted) {
                        afterKeyFound = true;
                        outFile << Key << L"=" << Value << std::endl;
                        keyInserted = true;
                    }
                } else {
                    outFile << line << std::endl;
                }
            }
            iniFile.close();
        }

        // 如果Section不存在，则创建新Section并添加键值对
        if (!sectionFound) {
            outFile << L"[" << Section << L"]" << std::endl;
            outFile << AfterKey << L"=" << std::endl;
            outFile << Key << L"=" << Value << std::endl;
            keyInserted = true;
        }
        // 如果找到Section但没找到AfterKey，则在Section末尾添加
        else if (!afterKeyFound && !keyInserted) {
            outFile << Key << L"=" << Value << std::endl;
        }

        outFile.close();

        // 替换原文件
        if (std::filesystem::exists(Path)) {
            std::filesystem::remove(Path);
        }

        if (std::filesystem::exists(Path + L".tmp")) {
            std::filesystem::rename(Path + L".tmp", Path);
            return true;
        }

        return false;
    }

    // 读取INI文件中指定Section和Key的值
    static std::wstring ReadValue(const std::wstring& Path, const std::wstring& Section, const std::wstring& Key) {
        std::wifstream iniFile(Path);
        std::wstring line;
        bool sectionFound = false;

        if (!iniFile.is_open()) {
            return L"";
        }

        while (std::getline(iniFile, line)) {
            if (!line.empty()) {
                if (line.find(L"[" + Section + L"]") != std::wstring::npos) {
                    sectionFound = true;
                    continue;
                }
                if (sectionFound) {
                    if (line.front() == L'[') {
                        iniFile.close();
                        return L"";
                    }
                    size_t equalPos = line.find(L"=");
                    if (equalPos != std::wstring::npos) {
                        std::wstring key = line.substr(0, equalPos);
                        if (key == Key) {
                            iniFile.close();
                            return line.substr(equalPos + 1);
                        }
                    }
                }
            }
        }
        iniFile.close();
        return L"";
    }

    // 写入或更新INI文件中指定Section和Key的值
    static bool WriteValue(const std::wstring& Path, const std::wstring& Section, const std::wstring& Key, const std::wstring& Value) {
        std::wifstream iniFile(Path);
        std::wstring line;
        bool sectionFound = false;
        bool keyFound = false;
        std::wofstream outFile(Path + L".tmp");

        if (!outFile.is_open()) {
            return false;
        }

        // 如果原文件存在且可以打开，则读取内容
        if (iniFile.is_open()) {
            while (std::getline(iniFile, line)) {
                if (line.empty() || std::all_of(line.begin(), line.end(), ::iswspace)) {
                    outFile << line << std::endl;
                    continue;
                }

                if (line.find(L"[" + Section + L"]") != std::wstring::npos) {
                    sectionFound = true;
                    outFile << line << std::endl;
                    continue;
                }

                if (!sectionFound) {
                    outFile << line << std::endl;
                    continue;
                }

                if (line.front() == L'[') {
                    if (!keyFound) {
                        outFile << Key << L"=" << Value << std::endl;
                        keyFound = true;
                    }
                    outFile << line << std::endl;
                    continue;
                }

                size_t equalPos = line.find(L"=");
                if (equalPos != std::wstring::npos) {
                    std::wstring currentKey = line.substr(0, equalPos);
                    if (currentKey == Key) {
                        outFile << Key << L"=" << Value << std::endl;
                        keyFound = true;
                    } else {
                        outFile << line << std::endl;
                    }
                } else {
                    outFile << line << std::endl;
                }
            }
            iniFile.close();
        }

        // 如果Section不存在或Key不存在，则添加
        if (!sectionFound) {
            outFile << L"[" << Section << L"]" << std::endl;
            outFile << Key << L"=" << Value << std::endl;
        } else if (!keyFound) {
            outFile << Key << L"=" << Value << std::endl;
        }

        outFile.close();

        // 替换原文件
        if (std::filesystem::exists(Path)) {
            std::filesystem::remove(Path);
        }

        if (std::filesystem::exists(Path + L".tmp")) {
            std::filesystem::rename(Path + L".tmp", Path);
            return true;
        }

        return false;
    }
};