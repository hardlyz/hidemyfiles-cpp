#include <iostream>
#include <string>
#include <vector>

#define NOMINMAX // анти идентификатор max (вызываем до виндовс.х + лимитс)

#include <Windows.h>
#include <sddl.h>
#include <cstdio>
#include <limits>
#include <fstream>
#include <Commdlg.h>
#include <stdexcept>

std::string gtaSaExeValue;


std::string GetUserSID() // можно сделать аналог тупо через HKEY_CURRENT_USER, но я перестраховался и сделал так.
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

std::vector<std::string> GetFilesInDirectory(const std::string& directory, const std::string& extension) // путь до папки + расширение файла
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

void PrintMenuOptions() // менюшка
{
	std::cout << "=== Меню \x1B[35mHide My Files\x1B[0m ===\n\n";
	std::cout << "1. \x1B[33mCLEO\x1B[0m скрипты\n";
	std::cout << "2. \x1B[34mLUA\x1B[0m скрипты\n";
	std::cout << "3. \x1B[36mSAMPFUNCS\x1B[0m плагины\n";
	std::cout << "4. \x1B[37mASI\x1B[0m плагины\n";
	std::cout << "5. \x1B[37mASI\x1B[0m плагины в папке scripts\n\n";
	std::cout << "==========================\n\n";
	std::cout << "6. Изменить путь до GTA:SA\n";
	std::cout << "7. \x1B[31mВыход\x1B[0m\n";
}

bool IsFileHidden(const std::string& filePath) // проверка 1
{
	DWORD attributes = GetFileAttributesA(filePath.c_str());
	return (attributes != INVALID_FILE_ATTRIBUTES) &&
		((attributes & FILE_ATTRIBUTE_HIDDEN) != 0) &&
		((attributes & FILE_ATTRIBUTE_SYSTEM) != 0);
}

bool HasFileAttribute(const std::string& filePath, DWORD attribute) // проверка 2
{
	DWORD fileAttributes = GetFileAttributesA(filePath.c_str());
	return (fileAttributes != INVALID_FILE_ATTRIBUTES) && ((fileAttributes & attribute) == attribute);
}

void UpdateGtaSaExePath(std::string& gtaSaExeValue) // указываем путь до gta_sa.exe
{
	OPENFILENAMEA ofn;
	char fileName[MAX_PATH] = ""; // буфер

	// инициализируем структуру 
	ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "Исполяемые файлы (*.exe)\0*.exe\0Все файлы (*.*)\0*.*\0"; // назначаем фильтр для файлов
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER; // флаги

	// Открытие диалогового окна
	if (GetOpenFileNameA(&ofn))
	{
		gtaSaExeValue = ofn.lpstrFile; // применяем к переменной путь до .ехе
		std::cout << "Путь до gta_sa.exe успешно обновлен: " << gtaSaExeValue << "\n";
		HKEY hKey;
		const char* subKey = "SOFTWARE\\SAMP";
		const char* valueName = "gta_sa_exe";
		std::string data = gtaSaExeValue;
		// сохраняем в дефолт путь сампа для упрощения жизни
		RegCreateKeyExA(HKEY_CURRENT_USER, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
		RegSetValueExA(hKey, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(data.c_str()), data.size());
		RegCloseKey(hKey);
	}
	else
	{
		std::cout << "Не удалось выбрать файл.\n";
	}
}

void PerformAction(const std::string& menuOption) // основная функция для взаимодействия с файлами
{
	system("cls");

	if (!gtaSaExeValue.empty())
	{
		std::string directory, extension, fileType;

		if (menuOption == "1") { // CLEO скрипты
			directory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/")) + "\\cleo";
			extension = ".cs";
			fileType = "CLEO скриптов";
		}
		else if (menuOption == "2") { // LUA скрипты
			directory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/")) + "\\moonloader";
			extension = ".lua";
			fileType = "LUA скриптов";
		}
		else if (menuOption == "3") { // SAMPFUNCS плагины
			directory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/")) + "\\sampfuncs";
			extension = ".sf";
			fileType = "SAMPFUNCS плагинов";
		}
		else if (menuOption == "4") { // ASI плагины
			directory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/"));
			extension = ".asi";
			fileType = "ASI плагинов";
		}
		else if (menuOption == "5") { // ASI плагины (scripts)
			directory = gtaSaExeValue.substr(0, gtaSaExeValue.find_last_of("\\/")) + "\\scripts";
			extension = ".asi";
			fileType = "ASI плагинов (scripts)";
		}
		else {
			std::cout << "Неверный выбор.\n";
			return;
		}

		std::vector<std::string> files = GetFilesInDirectory(directory, extension);

		std::cout << "Список " << fileType << ":\n\n";

		for (size_t i = 0; i < files.size(); ++i)
		{
			std::string fileInfo = files[i];
			if (IsFileHidden(directory + "\\" + fileInfo))
			{
				fileInfo += " \x1B[33m(скрыт)\x1B[0m"; // красиво выглядит кстати
			}
			std::cout << i + 1 << ". " << fileInfo << "\n";
		}

		if (!files.empty())
		{
			int choice = 0;
			std::cout << "\nВведите номер файла для взаимодействия или число больше списка файлов для выхода: ";
			std::cin >> choice;

			if (choice >= 1 && choice <= static_cast<int>(files.size()))
			{
				std::string selectedFile = files[choice - 1];
				std::string selectedFilePath = directory + "\\" + selectedFile;

				bool isHiddenAndSystem = IsFileHidden(selectedFilePath);
				bool isHidden = isHiddenAndSystem && HasFileAttribute(selectedFilePath, FILE_ATTRIBUTE_HIDDEN);
				bool isSystem = isHiddenAndSystem && HasFileAttribute(selectedFilePath, FILE_ATTRIBUTE_SYSTEM);
				if (!isHidden && !isSystem)
				{
					if (SetFileAttributesA(selectedFilePath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
					{
						std::cout << "Файл " << selectedFile << " теперь скрыт.\n";
						Sleep(625);
						system("cls");
						PrintMenuOptions();
					}
					else
					{
						std::cout << "Не удалось добавить атрибут скрытия для файла " << selectedFile << ".\n";
					}
				}
				else if (isHidden && isSystem)
				{
					if (SetFileAttributesA(selectedFilePath.c_str(), FILE_ATTRIBUTE_NORMAL))
					{
						std::cout << "Файл " << selectedFile << " теперь больше не скрыт.\n";
						Sleep(625);
						system("cls");
						PrintMenuOptions();
					}
					else
					{
						std::cout << "Не удалось снять атрибут скрытия для файла " << selectedFile << ".\n";
					}
				}
			}
			else
			{
				system("cls");
				PrintMenuOptions();
			}
		}
		else
		{
			std::cout << fileType << " не найдено.\n";
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

	std::string sampKeyPath = "Software\\SAMP";
	std::string registryKeyPath = userSID + "\\" + sampKeyPath;
	gtaSaExeValue = GetRegistryValue(registryKeyPath, "gta_sa_exe");

	while (true)
	{
		PrintMenuOptions();
		std::string choice;
		std::cout << "\nВыберите пункт меню: ";
		std::cin >> choice;

		if (gtaSaExeValue.empty())
		{
			std::cout << "Укажите путь к gta_sa.exe\n"; 
			UpdateGtaSaExePath(gtaSaExeValue);
		}

		if (choice == "1" || choice == "2" || choice == "3" || choice == "4" || choice == "5")
		{
			PerformAction(choice);
		}
		else if (choice == "6")
		{
			std::cout << "Укажите новый путь до gta_sa.exe: ";
			UpdateGtaSaExePath(gtaSaExeValue);
		}
		else if (choice == "7")
		{
			exit(0);
			break;
		}
		else
		{
			std::cout << "Неверный выбор.\n";
		}
		system("cls");
	}

	return 0;
}