#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <sddl.h>
#include <cstdio>
#include <limits>

std::string GetUserSID()
{
    DWORD len = 0;
    if (!GetUserName(nullptr, &len))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            std::vector<char> buffer(len);
            if (GetUserNameA(buffer.data(), &len))
            {
                HANDLE hToken;
                if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
                {
                    TOKEN_USER* ptu = nullptr;
                    DWORD bufferSize;
                    GetTokenInformation(hToken, TokenUser, nullptr, 0, &bufferSize);
                    ptu = (TOKEN_USER*)GlobalAlloc(GPTR, bufferSize);
                    if (ptu && GetTokenInformation(hToken, TokenUser, ptu, bufferSize, &bufferSize))
                    {
                        char* sidString = nullptr;
                        if (ConvertSidToStringSidA(ptu->User.Sid, &sidString))
                        {
                            std::string result(sidString);
                            LocalFree(sidString);
                            GlobalFree(ptu);
                            CloseHandle(hToken);
                            return result;
                        }
                    }
                    if (ptu)
                        GlobalFree(ptu);
                    CloseHandle(hToken);
                }
            }
        }
    }
    return std::string();
}

std::string GetRegistryValue(const std::string& keyPath, const std::string& valueName)
{
    std::string result;

    HKEY hKey;
    LSTATUS status = RegOpenKeyExA(HKEY_USERS, keyPath.c_str(), 0, KEY_READ, &hKey);
    if (status == ERROR_SUCCESS)
    {
        DWORD type;
        DWORD dataSize = 0;
        status = RegQueryValueExA(hKey, valueName.c_str(), nullptr, &type, nullptr, &dataSize);
        if (status == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ))
        {
            std::vector<char> rawData(dataSize + 1);
            status = RegQueryValueExA(hKey, valueName.c_str(), nullptr, &type, (LPBYTE)rawData.data(), &dataSize);
            if (status == ERROR_SUCCESS)
            {
                result.assign(rawData.data());
            }
        }
        RegCloseKey(hKey);
    }

    return result;
}

std::vector<std::string> GetFilesInDirectory(const std::string& directory, const std::string& extension)
{
    std::vector<std::string> files;

    std::string searchPattern = directory + "\\*" + extension;
    WIN32_FIND_DATAA fileData;
    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &fileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                files.push_back(fileData.cFileName);
            }
        } while (FindNextFileA(hFind, &fileData));

        FindClose(hFind);
    }

    return files;
}

void PrintMenuOptions()
{
    std::cout << "Меню:\n\n";
    std::cout << "1. CLEO\n";
    std::cout << "2. LUA\n";
    std::cout << "3. SAMPFUNCS\n";
    std::cout << "4. ASI\n\n";
    std::cout << "5. Выход\n";
}

void PerformCLEOAction()
{
    system("cls");
    std::string userSid = GetUserSID();
    std::string sampKeyPath = "Software\\SAMP";
    std::string gtaSaExeValue = GetRegistryValue(userSid + "\\" + sampKeyPath, "gta_sa_exe");
    if (!gtaSaExeValue.empty())
    {

        std::string cleoDirectory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/")) + "\\cleo";
        std::vector<std::string> cleoFiles = GetFilesInDirectory(cleoDirectory, ".cs");

        std::cout << "Список CLEO скриптов:\n\n";
        for (size_t i = 0; i < cleoFiles.size(); ++i)
        {
            std::cout << i + 1 << ". " << cleoFiles[i] << "\n";
        }

        if (!cleoFiles.empty())
        {
            int choice = 0;
            std::cout << "\nВведите номер файла для скрытия или число больше списка файлов для выхода: ";
            std::cin >> choice;

            if (choice >= 1 && choice <= static_cast<int>(cleoFiles.size()))
            {
                std::string selectedFile = cleoFiles[choice - 1];
                std::cout << "Interacting with file: " << selectedFile << "\n";
                std::string selectedFilePath = cleoDirectory + "\\" + selectedFile;
                if (SetFileAttributesA(selectedFilePath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
                {
                    std::cout << "Файл был скрыт.\n";
                }
                else
                {
                    std::cout << "Неудалось применить аттрибуты.\n";
                }
            }
            else
            {
                std::cout << "Неверный выбор.\n";
                system("cls");
            }
        }
        else
        {
            std::cout << "CLEO скриптов не обнаружено.\n";
        }
    }
    else
    {
        std::cout << "Путь до GTA:SA не был найден в реестре.\n";
    }
}

void PerformLUAAction()
{
    system("cls");
    std::string userSid = GetUserSID();
    std::string sampKeyPath = "Software\\SAMP";
    std::string gtaSaExeValue = GetRegistryValue(userSid + "\\" + sampKeyPath, "gta_sa_exe");
    if (!gtaSaExeValue.empty())
    {

        std::string luaDirectory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/")) + "\\moonloader";
        std::vector<std::string> luaFiles = GetFilesInDirectory(luaDirectory, ".lua");

        std::cout << "Список LUA скриптов:\n\n";
        for (size_t i = 0; i < luaFiles.size(); ++i)
        {
            std::cout << i + 1 << ". " << luaFiles[i] << "\n";
        }

        if (!luaFiles.empty())
        {
            int choice = 0;
            std::cout << "\nВведите номер файла для скрытия или число больше списка файлов для выхода: ";
            std::cin >> choice;

            if (choice >= 1 && choice <= static_cast<int>(luaFiles.size()))
            {
                std::string selectedFile = luaFiles[choice - 1];
                std::string selectedFilePath = luaDirectory + "\\" + selectedFile;
                if (SetFileAttributesA(selectedFilePath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
                {
                    std::cout << "Файл был скрыт.\n";
                }
                else
                {
                    std::cout << "Неудалось применить аттрибуты.\n";
                }
            }
            else
            {
                std::cout << "Неверный выбор.\n";
                system("cls");
            }
        }
        else
        {
            std::cout << "LUA скриптов не обнаружено.\n";
        }
    }
    else
    {
        std::cout << "Путь до GTA:SA не был найден в реестре.\n";
    }
}

void PerformSAMPFUNCSAction()
{
    system("cls");
    std::string userSid = GetUserSID();
    std::string sampKeyPath = "Software\\SAMP";
    std::string gtaSaExeValue = GetRegistryValue(userSid + "\\" + sampKeyPath, "gta_sa_exe");
    if (!gtaSaExeValue.empty())
    {

        std::string sampFuncsDirectory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/")) + "\\sampfuncs";
        std::vector<std::string> sampFuncsFiles = GetFilesInDirectory(sampFuncsDirectory, ".sf");

        std::cout << "Список SAMPFUNCS плагинов:\n\n";
        for (size_t i = 0; i < sampFuncsFiles.size(); ++i)
        {
            std::cout << i + 1 << ". " << sampFuncsFiles[i] << "\n";
        }

        if (!sampFuncsFiles.empty())
        {
            int choice = 0;
            std::cout << "\nВведите номер файла для скрытия или число больше списка файлов для выхода: ";
            std::cin >> choice;

            if (choice >= 1 && choice <= static_cast<int>(sampFuncsFiles.size()))
            {
                std::string selectedFile = sampFuncsFiles[choice - 1];
                std::string selectedFilePath = sampFuncsDirectory + "\\" + selectedFile;

                if (SetFileAttributesA(selectedFilePath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
                {
                    std::cout << "Файл был скрыт.\n";
                }
                else
                {
                    std::cout << "Неудалось применить аттрибуты.\n";
                }
            }
            else
            {
                std::cout << "Неверный выбор.\n";
                system("cls");
            }
        }
        else
        {
            std::cout << "SAMPFUNCS плагинов не найдено.\n";
        }
    }
    else
    {
        std::cout << "Путь до GTA:SA не был найден в реестре.\n";
    }
}

void PerformASIAction()
{

    system("cls");
    std::string userSid = GetUserSID();
    std::string sampKeyPath = "Software\\SAMP";
    std::string gtaSaExeValue = GetRegistryValue(userSid + "\\" + sampKeyPath, "gta_sa_exe");
    if (!gtaSaExeValue.empty())
    {

        std::string asiDirectory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/"));
        std::vector<std::string> asiFiles = GetFilesInDirectory(asiDirectory, ".asi");

        std::cout << "Список ASI плагинов:\n\n";
        for (size_t i = 0; i < asiFiles.size(); ++i)
        {
            std::cout << i + 1 << ". " << asiFiles[i] << "\n";
        }

        if (!asiFiles.empty())
        {
            int choice = 0;
            std::cout << "\nВведите номер файла для скрытия или число больше списка файлов для выхода: ";
            std::cin >> choice;

            if (choice >= 1 && choice <= static_cast<int>(asiFiles.size()))
            {
                std::string selectedFile = asiFiles[choice - 1];
                std::string selectedFilePath = asiDirectory + "\\" + selectedFile;
                if (SetFileAttributesA(selectedFilePath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
                {
                    std::cout << "Файл был скрыт.\n";
                    system("cls");
                }
                else
                {
                    std::cout << "Неудалось применить аттрибуты.\n";
                }
            }
            else
            {
                std::cout << "Неверный выбор.\n";
                system("cls");
            }
        }
        else
        {
            std::cout << "ASI плагинов не найдено.\n";
        }
    }
    else
    {
        std::cout << "Путь до GTA:SA не был найден в реестре.\n";
    }
}

int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    std::string userSID = GetUserSID();

    if (!userSID.empty())
    {
        std::string sampRegPath = userSID + "\\Software\\SAMP";
        std::string gtaSaExeValue = GetRegistryValue(sampRegPath, "gta_sa_exe");

        if (!gtaSaExeValue.empty())
        {
            int choice = 0;

            while (choice != 5)
            {
                PrintMenuOptions();
                std::cout << "\nВведите пункт меню: ";
                std::cin >> choice;

                switch (choice)
                {
                case 1:
                    system("cls");
                    PerformCLEOAction();
                    break;
                case 2:
                    system("cls");
                    PerformLUAAction();
                    break;
                case 3:
                    system("cls");
                    PerformSAMPFUNCSAction();
                    break;
                case 4:
                    system("cls");
                    PerformASIAction();
                    break;
                case 5:
                    system("cls");
                    exit(0);
                    break;
                default:
                    system("cls");
                    PrintMenuOptions();
                    break;
                }
            }
        }
        else
        {
            std::cout << "Путь до GTA:SA не был найден в реестре.\n";
        }
    }
    else
    {
        std::cout << "Не получилось получить идентификатор пользователя.\n";
    }

    return 0;
}