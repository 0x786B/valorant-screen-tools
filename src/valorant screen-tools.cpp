#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
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
        if (pe32.szExeFile == processName) {
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
        if (pe32.szExeFile == processName) {
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

// 备份配置文件
static void BackupConfig(const std::wstring& Path) {
    std::wcout << "[分辨率助手]: 正在备份配置文件..." << std::endl;
    std::wstring backupFilePath = Path + L".bak";
    if (std::filesystem::exists(backupFilePath)) {
        std::wcerr << "[分辨率助手]: 已存在备份,跳过备份..." << std::endl;
        return;
    }
    try {
        std::wifstream iniFile(Path);
        if (!iniFile.is_open()) {
            std::wcerr << "[错误]: 无法打开配置文件" << std::endl;
            return;
        }

        std::wofstream backupFile(backupFilePath);
        if (!backupFile.is_open()) {
            std::wcerr << "[错误]: 无法创建备份文件" << std::endl;
            return;
        }

        std::wstring line;
        while (std::getline(iniFile, line)) {
            backupFile << line << std::endl;
        }

        std::wcout << "[分辨率助手]: 配置文件已备份" << std::endl;
    }
    catch (const std::exception& e) {
        std::wcerr << L"[异常]: " << e.what() << std::endl;
    }
}
//读取配置
static std::wstring ReadConfig(const std::wstring& Path, const std::wstring& Section, const std::wstring& Key) {
    std::wifstream iniFile(Path);
    std::wstring line;
    bool sectionFound = false;
    if (!iniFile.is_open()) {
        std::wcerr << "[错误]: 无法打开配置文件,可能是被其他程序占用了。" << std::endl;
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
                    break;
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

//修改配置
static bool WriteConfig(const std::wstring& Path, const std::wstring& Section, const std::wstring& Key, const std::wstring& Value) {
    std::wifstream iniFile(Path);
    std::wstring line;
    bool sectionFound = false;
    bool keyFound = false;
    std::wofstream outFile(Path + L".tmp");
    if (!iniFile.is_open()) {
        std::wcerr << "[错误]: 无法打开配置文件,可能是被其他程序占用了。" << std::endl;
        return false;
    }
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
            outFile << line << std::endl;
            break;
        }
        size_t equalPos = line.find(L"=");
        if (equalPos != std::wstring::npos) {
            std::wstring currentKey = line.substr(0, equalPos);
            if (currentKey == Key) {
                keyFound = true;
                outFile << Key << L"=" << Value << std::endl;
            }
            else {
                outFile << line << std::endl;
            }
        }
        else {
            outFile << line << std::endl;
        }
    }
    if (!keyFound) {
        if (sectionFound) {
            outFile << Key << L"=" << Value << std::endl;
        }
        else {
            outFile << L"[" << Section << L"]" << std::endl;
            outFile << Key << L"=" << Value << std::endl;
        }
    }
    while (std::getline(iniFile, line)) {
        outFile << line << std::endl;
    }
    iniFile.close();
    outFile.close();

    if (std::filesystem::exists(Path)) {
        std::filesystem::remove(Path);
    }

    if (std::filesystem::exists(Path + L".tmp")) {
        std::filesystem::rename(Path + L".tmp", Path);
    }
    return true;
}

static void get_GameUserSettings(const std::wstring& Path) {
    const std::wstring section = L"/Script/ShooterGame.ShooterGameUserSettings";
    std::vector<std::pair<std::wstring, std::wstring>> settings = {
        {L"bShouldLetterbox", L""},
        {L"bLastConfirmedShouldLetterbox", L""},
        {L"ResolutionSizeX", L""},
        {L"ResolutionSizeY", L""},
        {L"LastUserConfirmedResolutionSizeX", L""},
        {L"LastUserConfirmedResolutionSizeY", L""},
        {L"LastConfirmedFullscreenMode", L""},
        {L"PreferredFullscreenMode", L""},
        {L"FullscreenMode", L""}
    };
    for (auto& setting : settings) {
        setting.second = ReadConfig(Path, section, setting.first);
        if (setting.second.empty()) {
            std::wcerr << L"[警告]: 未找到键 " << setting.first << L" 的值。" << std::endl;
        }
    }
    std::wcout << L"bShouldLetterbox: " << settings[0].second << std::endl;
    std::wcout << L"bLastConfirmedShouldLetterbox: " << settings[1].second << std::endl;
    std::wcout << L"ResolutionSizeX: " << settings[2].second << std::endl;
    std::wcout << L"ResolutionSizeY: " << settings[3].second << std::endl;
    std::wcout << L"LastUserConfirmedResolutionSizeX: " << settings[4].second << std::endl;
    std::wcout << L"LastUserConfirmedResolutionSizeY: " << settings[5].second << std::endl;
    std::wcout << L"LastConfirmedFullscreenMode: " << settings[6].second << std::endl;
    std::wcout << L"PreferredFullscreenMode: " << settings[7].second << std::endl;
    std::wcout << L"FullscreenMode: " << settings[8].second << std::endl;
}

static void EnsureFullscreenMode(const std::wstring& Path) {
    std::wcout << "[分辨率助手]: 正在修补配置文件缺失的FullscreenMode项..." << std::endl;
    try {
        std::wifstream file(Path);
        std::vector<std::wstring> lines;
        std::wstring line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        size_t startIndex = lines.size();
        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].find(L"HDRDisplayOutputNits") != std::wstring::npos) {
                startIndex = i;
                break;
            }
        }
        size_t endIndex = lines.size();
        for (size_t i = startIndex + 1; i < lines.size(); ++i) {
            if (!lines[i].empty() && lines[i].front() == L'[') {
                endIndex = i;
                break;
            }
        }
        bool fullscreenModeExists = false;
        for (size_t i = startIndex; i < endIndex; ++i) {
            if (lines[i].find(L"FullscreenMode") != std::wstring::npos) {
                fullscreenModeExists = true;
                break;
            }
        }
        if (!fullscreenModeExists) {
            lines.insert(lines.begin() + endIndex - 1, L"FullscreenMode=2");
            std::wofstream outFile(Path);
            for (const auto& l : lines) {
                outFile << l << std::endl;
            }
            outFile.close();
            std::wcout << "[分辨率助手]: 已修补FullscreenMode项" << std::endl;
        }
        else {
            std::wcout << "[分辨率助手]: 已存在FullscreenMode项,跳过修补..." << std::endl;
        }

    }
    catch (const std::exception& e) {
        std::wcerr << L"[Error]: 修改GameUserSettings.ini文件时发生错误: " << e.what() << std::endl;
    }
}


static void update_GameUserSettings(const std::wstring& Path, const std::wstring& X, const std::wstring& Y) {
    std::wcout << "[分辨率助手]: 正在修改配置文件..." << std::endl;
    const std::wstring section = L"/Script/ShooterGame.ShooterGameUserSettings";
    std::vector<std::pair<std::wstring, std::wstring>> settingsToUpdate = {
        {L"bShouldLetterbox", L"False"},
        {L"bLastConfirmedShouldLetterbox", L"False"},
        {L"ResolutionSizeX", X},
        {L"ResolutionSizeY", Y},
        {L"LastUserConfirmedResolutionSizeX", X},
        {L"LastUserConfirmedResolutionSizeY", Y},
        {L"LastConfirmedFullscreenMode", L"2"},
        {L"PreferredFullscreenMode", L"2"},
        {L"FullscreenMode", L"2"}
    };
    for (const auto& setting : settingsToUpdate) {
        if (!WriteConfig(Path, section, setting.first, setting.second)) {
            std::wcerr << L"[错误]: 更新配置失败。" << std::endl;
            return;
        }
    }
}

//获取屏幕分辨率
static std::pair<int, int> getScreenResolution() {
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    if (GetWindowRect(hDesktop, &desktop)) {
        return { desktop.right - desktop.left, desktop.bottom - desktop.top };
    }
    return { 0, 0 };
}

int main() {
    SetConsoleTitle(L"VALORANT分辨率助手 1.0.0-C++ Edition");
    if (!IsProcessRunning(L"VALORANT.exe")) {
        MessageBox(NULL, L"请先启动VALORANT客户端", L"VALORANT分辨率助手", MB_OK);
        return 0;
    }
    std::wcout << "欢迎VALORANT分辨率助手,目前仅支持国服!!!" << std::endl;
    std::wstring processName = L"VALORANT.exe";
    std::wstring directory = GetProcessDirectory(processName);
    std::wstring ConfigPath = directory + L"ShooterGame\\Saved\\Config";
    std::wstring RiotLocalMachine = ConfigPath + L"\\Windows\\RiotLocalMachine.ini";
    std::wstring lastKnownUser = ReadConfig(RiotLocalMachine, L"UserInfo", L"LastKnownUser");
    if (lastKnownUser.empty()) {
        std::wcout << "[分辨率助手]: 没有找到登录信息,请确认登录过一次" << std::endl;
        std::wcout << "[分辨率助手]: 按回车键退出";
        std::cin.get();
        return 0;
    }
    std::wcout << "[分辨率助手]: " << lastKnownUser << std::endl;
    std::wcout << "[分辨率助手]: 请手动关闭VALORANT客户端。" << std::endl;
    std::wcout << "[分辨率助手]: 正在等待客户端关闭..." << std::endl;
    std::wstring Settingdirectory = ConfigPath + L"\\" + lastKnownUser + L"-alpha1\\Windows";
    std::wstring SettingPath = Settingdirectory + L"\\GameUserSettings.ini";
    while (IsProcessRunning(processName)) {
        Sleep(1000);
    }
    std::wcout << "[分辨率助手]: 客户端已关闭,修改期间请勿启动游戏!!!" << std::endl;
    std::wcout << "[分辨率助手]: 开始执行后续操作..." << std::endl;
    BackupConfig(SettingPath);
    EnsureFullscreenMode(SettingPath);
    std::wcout << "未修改的配置信息" << std::endl;
    get_GameUserSettings(SettingPath);
    std::wcout << "[分辨率助手]: 如果需要开始修改分辨率请按下回车继续..." << std::endl;
    std::cin.get();
    std::wcout << "[分辨率助手]: 请仔细阅读以下注意事项！！！" << std::endl;
    std::wcout << "[分辨率助手]: 请将桌面分辨率调整为您需要的分辨率" << std::endl;
    std::wcout << "[分辨率助手]: 例如1440X1080,1280X1080,1280X1024等" << std::endl;
    std::wcout << "[分辨率助手]: 如需拉伸画面请进入NVIDIA控制面板进行设置" << std::endl;
    std::wcout << "[分辨率助手]: 显示-调整桌面尺寸和位置-缩放-全屏" << std::endl;
    std::wcout << "[分辨率助手]: 对以下项目执行缩放-GPU" << std::endl;
    std::wcout << "[分辨率助手]: 勾选-覆盖由游戏和程序设置的缩放模式" << std::endl;
    std::wcout << "[分辨率助手]: 如果已经完成上述操作,请按回车继续..." << std::endl;
    std::cin.get();
    std::pair<int, int> screenResolution = getScreenResolution();
    std::wcout << "[分辨率助手]: 游戏将修改分辨率为 " << screenResolution.first << "x" << screenResolution.second << std::endl;
    std::wcout << "[分辨率助手]: 如需修改请重新运行该程序" << std::endl;
    std::wcout << "[分辨率助手]: 确认修改请按回车键继续...";
    std::cin.get();
    update_GameUserSettings(SettingPath, std::to_wstring(screenResolution.first), std::to_wstring(screenResolution.second));
    std::wcout << "修改后的配置信息" << std::endl;
    get_GameUserSettings(SettingPath);
    std::wcout << "[分辨率助手]: 配置已修改完成!!!" << std::endl;
    std::wcout << "[分辨率助手]: 您可以将GameUserSettings.ini设置为只读。" << std::endl;
    std::wcout << "[分辨率助手]: 这样可以防止下次启动被客户端覆盖。" << std::endl;
    std::wcout << "[分辨率助手]: 如需恢复请删除GameUserSettings.ini并且将bak文件.bak删除即可恢复。" << std::endl;
    std::wcout << "[分辨率助手]: 请关闭程序后再启动游戏！！！" << std::endl;
    std::wcout << "[分辨率助手]: 请关闭程序后再启动游戏！！！" << std::endl;
    std::wcout << "[分辨率助手]: 请关闭程序后再启动游戏！！！" << std::endl;
    std::wcout << "[分辨率助手]: 按回车键退出程序...";
    std::cin.get();
    ShellExecute(NULL, L"open", Settingdirectory.c_str(), NULL, NULL, SW_SHOWNORMAL);
    ShellExecute(NULL, L"open", SettingPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return 0;
}

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Kernel32.lib")