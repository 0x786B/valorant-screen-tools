import os
import time
import ctypes
import shutil
import psutil
import configparser

class CustomConfigParser(configparser.ConfigParser):
    def optionxform(self, optionstr):
        return optionstr 


def get_process_path(process_name):
    try:
        for proc in psutil.process_iter(['name', 'exe']):
            if proc.info['name'] == process_name:
                return proc.info['exe']
    except (psutil.NoSuchProcess, psutil.AccessDenied):
        print(f"[Error]: 无法访问进程信息.")
    return None

def check_process(process_name):
    try:
        return any(process_name.lower() in proc.info['name'].lower() for proc in psutil.process_iter(['name']))
    except (psutil.NoSuchProcess, psutil.AccessDenied):
        return False

def read_ini_value(ini_file_path, section, option):
    config = CustomConfigParser()
    try:
        config.read(ini_file_path)
        if config.has_option(section, option):
            return config.get(section, option)
    except Exception as e:
        print(f"[Error]: 读取INI文件 {ini_file_path} 时发生错误: {e}")
    return None

def read_specific_settings(settings_path):
    settings_to_read = [
        "bShouldLetterbox",
        "bLastConfirmedShouldLetterbox",
        "ResolutionSizeX",
        "ResolutionSizeY",
        "LastUserConfirmedResolutionSizeX",
        "LastUserConfirmedResolutionSizeY",
        "LastConfirmedFullscreenMode",
        "PreferredFullscreenMode",
        "FullscreenMode"
    ]

    settings = {}
    try:
        with open(settings_path, 'r', encoding='utf-8') as file:
            for line in file:
                line = line.strip()
                if '=' in line:
                    key, value = line.split('=', 1)
                    key = key.strip()
                    value = value.strip()

                    if key in settings_to_read:
                        settings[key] = value
        
        print("[分辨率助手]: 配置信息")
        for key in settings_to_read:
            if key in settings:
                print(f"{key}={settings[key]}")
            else:
                print(f"{key} 的值未找到")
    except FileNotFoundError:
        print("[Error]: 找不到GameUserSettings.ini文件.")
    except Exception as e:
        print(f"[Error]: 读取GameUserSettings.ini文件时发生错误: {e}")

def backup_settings_file(original_path):
    backup_path = f"{original_path}.bak"
    if not os.path.exists(backup_path):
        try:
            shutil.copyfile(original_path, backup_path)
            print(f"[分辨率助手]: 已自动备份")
        except Exception as e:
            print(f"[Error]: 备份文件时发生错误: {e}")
    else:
        print(f"[分辨率助手]: 已存在备份文件,自动跳过")

def insert_fullscreen_mode(settings_path):
    try:
        with open(settings_path, 'r', encoding='utf-8') as file:
            lines = file.readlines()
        start_index = None
        end_index = None
        for i, line in enumerate(lines):
            if 'HDRDisplayOutputNits' in line:
                start_index = i
            elif start_index is not None and line.startswith('['):
                end_index = i
                break
        if start_index is not None and end_index is None:
            end_index = len(lines)
        fullscreen_mode_exists = any('FullscreenMode' in line for line in lines[start_index:end_index])
        if not fullscreen_mode_exists:
            lines.insert(end_index - 1, 'FullscreenMode=2\n\n')
        with open(settings_path, 'w', encoding='utf-8') as file:
            file.writelines(lines)
        if not fullscreen_mode_exists:
            print("[分辨率助手]: 已修补配置项")
        else:
            print("[分辨率助手]: 无需修补...")

    except Exception as e:
        print(f"[Error]: 修改GameUserSettings.ini文件时发生错误: {e}")

def update_settings(settings_path, width, height):
    config = CustomConfigParser()  
    config.read(settings_path)
    if not config.has_section('/Script/ShooterGame.ShooterGameUserSettings'):
        config.add_section('/Script/ShooterGame.ShooterGameUserSettings')
    config['/Script/ShooterGame.ShooterGameUserSettings']['bShouldLetterbox'] = 'False'
    config['/Script/ShooterGame.ShooterGameUserSettings']['bLastConfirmedShouldLetterbox'] = 'False'
    config['/Script/ShooterGame.ShooterGameUserSettings']['ResolutionSizeX'] = str(width)
    config['/Script/ShooterGame.ShooterGameUserSettings']['ResolutionSizeY'] = str(height)
    config['/Script/ShooterGame.ShooterGameUserSettings']['LastUserConfirmedResolutionSizeX'] = str(width)
    config['/Script/ShooterGame.ShooterGameUserSettings']['LastUserConfirmedResolutionSizeY'] = str(height)
    config['/Script/ShooterGame.ShooterGameUserSettings']['LastConfirmedFullscreenMode'] = '2'
    config['/Script/ShooterGame.ShooterGameUserSettings']['PreferredFullscreenMode'] = '2'
    config['/Script/ShooterGame.ShooterGameUserSettings']['FullscreenMode'] = '2'
    with open(settings_path, 'w', encoding='utf-8') as configfile:
        config.write(configfile)
    print("[分辨率助手]: 配置文件已更新")

def main():
    print("欢迎VALORANT分辨率助手,当前版本: 1.0.0")
    process_name = "VALORANT.exe"
    path = get_process_path(process_name)
    
    if not path:
        print("[分辨率助手]: 请先启动VALORANT客户端!")
        return
    print(f"[分辨率助手]: {path}")

    ini_file_path = os.path.join(os.path.dirname(path), "ShooterGame", "Saved", "Config", "Windows", "RiotLocalMachine.ini")
    last_known_user = read_ini_value(ini_file_path, 'UserInfo', 'LastKnownUser')
    
    if not last_known_user:
        print("[分辨率助手]: 没有找到登录信息,请确认登录过一次")
        return
    print(f"[分辨率助手]: {last_known_user}")

    settings_path = os.path.join(os.path.dirname(path), "ShooterGame", "Saved", "Config", f"{last_known_user}-alpha1", "Windows", "GameUserSettings.ini")
    
    if not os.path.exists(settings_path):
        print("[分辨率助手]: 没有找到配置文件,请确认修改过一次")
        return
    print(f"[分辨率助手]: {settings_path}")
    print("[分辨率助手]: 请手动关闭VALORANT客户端完成后续步骤")
    while check_process(process_name):
        time.sleep(5)

    backup_settings_file(settings_path)
    insert_fullscreen_mode(settings_path)
    read_specific_settings(settings_path)

    print("[分辨率助手]: 请将桌面分辨率调整为您需要的分辨率")
    print("[分辨率助手]: 例如1440X1080,1280X1080,1280X1024等")
    print("[分辨率助手]: 如需拉伸画面请将NVIDIA控制面板的设置进行调整")
    print("[分辨率助手]: N卡控制面板-显示-调整桌面尺寸和位置")
    print("[分辨率助手]: 选择缩放模式设置为全屏")
    print("[分辨率助手]: 对以下项目执行缩放设置为GPU")
    print("[分辨率助手]: [√]覆盖由游戏和程序设置的缩放模式")
    print("[分辨率助手]: 如果已经完成上述操作,请按任意键继续...")
    input()

    user32 = ctypes.windll.user32
    screensize = user32.GetSystemMetrics(0), user32.GetSystemMetrics(1)
    print(f"[分辨率助手]: 当前屏幕分辨率为 {screensize[0]}x{screensize[1]}")


    update_settings(settings_path, screensize[0], screensize[1])
    read_specific_settings(settings_path)

    print("[分辨率助手]: 分辨率设置已更新")
    print("[分辨率助手]: 该配置仅针对当前登录的账号有效")
    print("[分辨率助手]: 你可以尝试将当前账号的配置文件复制到其他账号的目录下")
    print("[分辨率助手]: 不想重复操作可以将GameUserSettings.ini设置为只读")
    print("[分辨率助手]: 或许可以避免被游戏客户端覆盖配置导致失效")
    print("[分辨率助手]: 如需要恢复原有设置,请将备份文件GameUserSettings.ini.bak复制到原有位置并覆盖原有文件")
    print("[分辨率助手]: 请勿在游戏过程中修改分辨率相关的设置，否则可能会失效。")
    os.startfile(os.path.dirname(settings_path))
    time.sleep(5)
    os._exit(0)

if __name__ == "__main__":
    main()