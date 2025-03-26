#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <Shlwapi.h>
#include <locale>
#include <codecvt>
#include <limits>
#include "IniHelper.h"
using namespace std;
//进程是否在运行
static bool IsProcessRunning(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return false;
    }

    do {
        if (wcscmp(pe32.szExeFile, processName.c_str()) == 0) {
            CloseHandle(hSnapshot);
            return true;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return false;
}
//获取进程的目录
static std::wstring GetProcessDirectory(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return L"";
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return L"";
    }

    do {
        if (wcscmp(pe32.szExeFile, processName.c_str()) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess == NULL) {
                CloseHandle(hSnapshot);
                return L"";
            }

            WCHAR path[MAX_PATH];
            DWORD pathLength = GetModuleFileNameEx(hProcess, NULL, path, MAX_PATH);
            CloseHandle(hProcess);
            if (pathLength > 0 && pathLength < MAX_PATH) {
                std::wstring directoryPath(path);
                directoryPath = directoryPath.substr(0, directoryPath.find_last_of(L"\\") + 1);
                CloseHandle(hSnapshot);
                return directoryPath;
            }
            else {
                CloseHandle(hSnapshot);
                return L"";
            }
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return L"";
}

//获取配置文件夹列表
static std::vector<std::wstring> GetConfigDirectoryList(const std::wstring& directoryPath) {
    std::vector<std::wstring> configDirs;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_directory()) {
                std::wstring dirName = entry.path().filename().wstring();
                // 检查文件夹名是否以-alpha1结尾
                if (dirName.length() >= 7 && 
                    dirName.substr(dirName.length() - 7) == L"-alpha1") {
                    // 只添加文件夹名称，不包含完整路径
                    configDirs.push_back(dirName);
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::wcerr << L"[错误]: 获取配置文件夹列表时发生错误: " << e.what() << std::endl;
    }
    return configDirs;
}

//判断文件或目录是否存在
static bool IsFileExists(const std::wstring& filePath) {
    return std::filesystem::exists(filePath);
}

//判断目录是否为空
static bool IsDirectoryEmpty(const std::wstring& directoryPath) {
    return std::filesystem::is_empty(directoryPath);
}

//获取屏幕分辨率
static std::pair<int, int> Get_ScreenResolution() {
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    if (GetWindowRect(hDesktop, &desktop)) {
        return { desktop.right - desktop.left, desktop.bottom - desktop.top };
    }
    return { 0, 0 };
}

int main() {
    wcout.imbue(std::locale("zh_CN"));
    SetConsoleTitle(L"VALORANT分辨率助手 2.0 C++ Edition");
    std::wcout << L"欢迎使用VALORANT分辨率助手" << std::endl;
    std::wcout << L"免责声明: 如果您使用本程序造成的任何问题，与作者无关，后果自负。" << std::endl;
    std::wcout << L"免责声明: 本程序仅供学习交流,请勿用于非法用途,请在下载后24小时内删除。" << std::endl;
    std::wcout << L"免责声明: 如果您使用本程序就代表着您同意以上声明，作者将无需承担任何责任。" << std::endl;
    std::wcout << L"按下回车继续...";
    std::cin.get();
    system("cls");
    std::wcout << L"欢迎使用VALORANT分辨率助手" << std::endl;
    std::wcout << L"当前版本:2.0.0" << std::endl;
    std::wcout << L"现已支持单独修改/批量修改" << std::endl;
    std::wcout << L"如果您禁用过监视器,请重新启用监视器" << std::endl;
    std::wcout << L"重新启用监视器后,请先启动一次游戏恢复配置" << std::endl;
    std::wcout << L"再使用本程序进行修改,否则可能会无法正常修改" << std::endl;
    std::wcout << L"按下回车继续...";
    std::cin.get();
    system("cls");
    std::wstring processName = L"VALORANT.exe";
    if (!IsProcessRunning(processName)) {
        std::wcout << L"[分辨率助手] 检测到游戏未运行!!!" << std::endl;
        std::wcout << L"[分辨率助手] 请登录您要修改分辨率的账号" << std::endl;
        std::wcout << L"[分辨率助手] 在运行游戏后将会自动进行下一步" << std::endl;
        std::wcout << L"[分辨率助手] 正在等待游戏运行...";
        while (!IsProcessRunning(processName)) {
            Sleep(1000);
        }
    }
    system("cls");
    std::wstring directory = GetProcessDirectory(processName);
    std::wcout << L"[分辨率助手] 游戏已在运行中!!!" << std::endl;
    std::wcout << L"[分辨率助手] 请进入大厅后手动退出游戏"<< std::endl;
    std::wcout << L"[分辨率助手] 游戏退出后将会自动进行下一步" << std::endl;
    std::wcout << L"[分辨率助手] 正在等待游戏结束...";
    while (IsProcessRunning(processName)) {
        Sleep(1000);
    }
    system("cls");
    std::wstring ConfigPath = directory + L"ShooterGame\\Saved\\Config";
    std::vector<std::wstring> ConfigPathList = GetConfigDirectoryList(ConfigPath);
    std::wstring RiotLocalMachine = ConfigPath + L"\\Windows\\RiotLocalMachine.ini";
    std::wstring lastKnownUser = IniHelper::ReadValue(RiotLocalMachine, L"UserInfo", L"LastKnownUser");
    std::wstring Settingdirectory = ConfigPath + L"\\" + lastKnownUser + L"-alpha1\\Windows";
    std::wstring SettingPath = Settingdirectory + L"\\GameUserSettings.ini";
    if (!IsFileExists(RiotLocalMachine) || lastKnownUser.empty() || ConfigPathList.empty() || !IsFileExists(SettingPath)) {
        std::wcout << L"[分辨率助手]: 未检测到当前登录账号的数据或是数据不完整" << std::endl;
        std::wcout << L"[分辨率助手]: 请关闭该程序后,登录一次账号,进入靶场后退出游戏。" << std::endl;
        std::wcout << L"[分辨率助手]: 完成以上步骤后可以重新运行程序尝试修改" << std::endl;
        std::wcout << L"[分辨率助手]: 按下回车结束程序...";
        std::cin.get();
        return 0;
    }
    std::wcout << L"[分辨率助手]: 登录账号:" << lastKnownUser << std::endl;
    std::wcout << L"[分辨率助手]: 配置文件夹:" << ConfigPath << std::endl;
    std::wcout << L"[分辨率助手]: 当前账号配置文件:" << RiotLocalMachine << std::endl;
    std::wcout << L"[分辨率助手]: 已登录过账号数量:" << ConfigPathList.size() << std::endl;
    std::wcout << L"[分辨率助手]: 请选择您要修改的方式:" << std::endl;
    std::wcout << L"[分辨率助手]: 1.仅修改当前登录账号" << std::endl;
    std::wcout << L"[分辨率助手]: 2.批量修改全部登录过账号" << std::endl;
    std::wcout << L"[分辨率助手]: 请选择(1/2):";
    int choice;
    while (true) {
        std::wcin >> choice;
        if (std::wcin.fail()) {
            std::wcin.clear();
            std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::wcout << L"[分辨率助手]: 输入错误,请重新输入(1/2):";
            continue;
        }
        if (choice != 1 && choice != 2) {
            std::wcout << L"[分辨率助手]: 输入错误,请重新输入(1/2):";
            continue;
        }
        break;
    }
    system("cls");
    std::wcout << L"[分辨率助手]: 请将桌面分辨率调整为您所需要的分辨率" << std::endl;
    std::wcout << L"[分辨率助手]: 例如1440X1080,1280X1080,1280X960" << std::endl << std::endl;
    std::wcout << L"[分辨率助手]: 关于如何自定义分辨率,以及解决黑边问题,请观看作者的教程视频" << std::endl;
    std::wcout << L"[分辨率助手]: 请在切换分辨率后,按下回车继续...";
    std::cin.get();
    std::cin.get();
    system("cls");
    std::pair<int, int> currentResolution = Get_ScreenResolution();
    std::wstring Section = L"/Script/ShooterGame.ShooterGameUserSettings";
    if (choice == 1) {
        // 修改当前登录账号的分辨率设置
        IniHelper::WriteValue(SettingPath, Section, L"bShouldLetterbox", L"false");
        IniHelper::WriteValue(SettingPath, Section, L"bLastConfirmedShouldLetterbox", L"false");
        IniHelper::WriteValue(SettingPath, Section, L"ResolutionSizeX", std::to_wstring(currentResolution.first));
        IniHelper::WriteValue(SettingPath, Section, L"ResolutionSizeY", std::to_wstring(currentResolution.second));
        IniHelper::WriteValue(SettingPath, Section, L"LastUserConfirmedResolutionSizeX", std::to_wstring(currentResolution.first));
        IniHelper::WriteValue(SettingPath, Section, L"LastUserConfirmedResolutionSizeY", std::to_wstring(currentResolution.second));
        IniHelper::WriteValue(SettingPath, Section, L"LastConfirmedFullscreenMode", L"2");
        IniHelper::WriteValue(SettingPath, Section, L"bLastConfirmedShouldLetterbox", L"2");
        IniHelper::WriteValue(SettingPath, Section, L"FullscreenMode", L"2");
        std::wcout << L"[分辨率助手]: 当前分辨率:" << currentResolution.first << L"X" << currentResolution.second << std::endl;
        std::wcout << L"[分辨率助手]: 修改成功,请关闭本程序后运行游戏。" << std::endl;
        // 打开设置文件夹和文件
        ShellExecute(NULL, L"open", Settingdirectory.c_str(), NULL, NULL, SW_SHOWNORMAL);
        ShellExecute(NULL, L"open", SettingPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    } else {
        // 批量修改所有账号的分辨率设置
        std::wcout << L"[分辨率助手]: 正在批量修改所有登录过账号的分辨率" << std::endl;
        for (const auto& configDir : ConfigPathList) {
            std::wstring SettingPath = ConfigPath + L"\\" + configDir + L"\\Windows\\GameUserSettings.ini";
            IniHelper::WriteValue(SettingPath, Section, L"bShouldLetterbox", L"false");
            IniHelper::WriteValue(SettingPath, Section, L"bLastConfirmedShouldLetterbox", L"false");
            IniHelper::WriteValue(SettingPath, Section, L"ResolutionSizeX", std::to_wstring(currentResolution.first));
            IniHelper::WriteValue(SettingPath, Section, L"ResolutionSizeY", std::to_wstring(currentResolution.second));
            IniHelper::WriteValue(SettingPath, Section, L"LastUserConfirmedResolutionSizeX", std::to_wstring(currentResolution.first));
            IniHelper::WriteValue(SettingPath, Section, L"LastUserConfirmedResolutionSizeY", std::to_wstring(currentResolution.second));
            IniHelper::WriteValue(SettingPath, Section, L"LastConfirmedFullscreenMode", L"2");
            IniHelper::WriteValue(SettingPath, Section, L"bLastConfirmedShouldLetterbox", L"2");
            IniHelper::WriteValue(SettingPath, Section, L"FullscreenMode", L"2");
            std::wcout << L"[分辨率助手]:" << configDir << L" 修改成功" << std::endl;
        }
        std::wcout << L"[分辨率助手]: 当前分辨率:" << currentResolution.first << L"X" << currentResolution.second << std::endl;
        std::wcout << L"[分辨率助手]: 全部修改成功,请关闭本程序后运行游戏。" << std::endl;
        // 打开配置文件夹
        ShellExecute(NULL, L"open", ConfigPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    
    // 通用提示信息
    std::wcout << L"[分辨率助手]: 平时不玩的时候可以切回正常的分辨率" << std::endl;
    std::wcout << L"[分辨率助手]: 游玩的过程中切换分辨率会导致拉伸失效" << std::endl;
    std::wcout << L"[分辨率助手]: 游玩过程中请保持修改分辨率与桌面分辨率一致" << std::endl;
    std::wcout << L"[分辨率助手]: 按下回车结束程序...";
    std::cin.get();
    return 0;
}
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Shlwapi.lib")