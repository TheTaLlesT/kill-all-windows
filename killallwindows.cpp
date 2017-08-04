#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <algorithm>

#include <windows.h>
#include <tlhelp32.h>
#include <conio.h>

#include "version.h"

#define BLUE 9
#define GREEN 10
#define RED 12
#define YELLOW 14
#define DEFAULT_COLOR 7

using namespace std;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
void LoadAllowedProcessList();
void GetRunningProcesses();
void AddToWindowList(HWND);
void KillWindows();

list<string> lAllowedProcesses; // List of allowed process names
list<unsigned int> lIgnoredPIDs; // List of killed/ignored pids
bool test; // Whether to actually kill
bool close; // send WM_CLOSE instead of killing prosess
HANDLE hConsole;

struct ProcessData
{
	string name;
    unsigned int PID;
    bool killed;
}; // Struct to hold process data in a vector

struct WindowData
{
    HWND hwnd;
    unsigned int PID;
	string title;
}; // Struct to hold window data in a vector

vector<ProcessData> vRunningProcesses; // List of running processes
vector<WindowData> vWindowData; // List of running processes

/* -------------------------------------------------------------------------- *
*    Win API main
*  -------------------------------------------------------------------------- */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int iCmdShow)
{
	AllocConsole();
	freopen("CONOUT$", "w+", stdout);
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    close = false;
    #if DEBUG
        test = true;
    #else
        if (strcmp(&pCmdLine[1], "-kill") != 0 && strcmp(&pCmdLine[1], "-close") != 0 && strcmp(&pCmdLine[1], "-test") != 0)
        {
            cout << "Window Killer " << AutoVersion::FULLVERSION_STRING << endl
                 << endl
                 << "Options:" << endl
                 << "\t--close\tSends WM_CLOSE to all visible windows" << endl
                 << "\t--kill\tKills all visible windows" << endl
                 << "\t--test\tShows what would be killed without actually doing it" << endl
                 << endl
                 << "Press any key to close...";
            getch();
            return 0;
        }
        else if (strcmp(&pCmdLine[1], "-close") == 0)
        {
            close = true;
            test = false;
        }
        else if (strcmp(&pCmdLine[1], "-test") == 0)
        {
            test = true;
            SetConsoleTextAttribute(hConsole, RED); // RED text
            cout << "TEST MODE" << endl << "Shows what would be killed without actually doing it" << endl << endl;
            SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text
        }
        else
            test = false;
    #endif
    lIgnoredPIDs.push_back((unsigned int)GetCurrentProcessId()); // Ignore thie pid of this process so we dont kill ourselfs NOTE: only valid if launched directly otherwise window will be owned by console

    // ignore desktop window pid
    DWORD processId;
    GetWindowThreadProcessId(GetShellWindow(), &processId);
    lIgnoredPIDs.push_back((unsigned int)processId);

    SetConsoleTextAttribute(hConsole, YELLOW);
    cout << "Loading Allowed Processes List..." << endl;
    SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text
    LoadAllowedProcessList();
    SetConsoleTextAttribute(hConsole, GREEN); // GREEN text
    cout << "Done." << endl << endl;

    SetConsoleTextAttribute(hConsole, YELLOW);
    cout << "Getting Running Processes..." << endl;
    GetRunningProcesses();
    SetConsoleTextAttribute(hConsole, GREEN); // GREEN text
    cout << "Done." << endl << endl;

    SetConsoleTextAttribute(hConsole, YELLOW);
    cout << "Searching for Windows..." << endl;
	EnumWindows(EnumWindowsProc, '\0'); // Win API call to retrun windows
	SetConsoleTextAttribute(hConsole, GREEN); // GREEN text
    cout << "Done." << endl << endl;

    SetConsoleTextAttribute(hConsole, YELLOW);
    cout << "Killing Windows..." << endl << endl;
    KillWindows();
    SetConsoleTextAttribute(hConsole, GREEN); // GREEN text
    cout << "Done." << endl << endl;
    SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text

    cout << "Press any key to close...";
    getch();
    return 0;
}

/* -------------------------------------------------------------------------- *
*    Win API callback of a found window
*  -------------------------------------------------------------------------- */
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if(IsWindowVisible(hwnd)>0 && IsWindow(hwnd)>0) // kill only if window is visable
        AddToWindowList(hwnd);

	return TRUE;
}

/* -------------------------------------------------------------------------- *
*    Creates a list of windows
*  -------------------------------------------------------------------------- */
void AddToWindowList(HWND hwnd)
{
	char title[60];
	DWORD processId;
    WindowData tmpWindowData;

    GetWindowThreadProcessId(hwnd, &processId); // Get window owner pid

    if (processId == 0) // Dont add any system prosesses to the list(should not happen)
        return;

    memset(title, '\0', sizeof(title)); // null string to prevent bad output
    GetWindowText(hwnd,title,sizeof(title)); // get window title
    tmpWindowData.hwnd = hwnd;
    tmpWindowData.PID = processId;
    tmpWindowData.title = title;
    if(tmpWindowData.title.length() >= 59)
        tmpWindowData.title.append("...");
    vWindowData.push_back(tmpWindowData);
    #if DEBUG
    SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text
    cout << "    " << processId << "    \t" << tmpWindowData.title << endl;
    #endif
}

/* -------------------------------------------------------------------------- *
*    Loads a list of allowed processes from a file
*  -------------------------------------------------------------------------- */
void LoadAllowedProcessList()
{
    string line;
    ifstream myfile("AllowedProcesses.cfg");

    if (myfile.is_open())
    {
        while ( myfile.good() )
        {
            getline (myfile,line);
            if (line.length()>1 && line[0]!= '#' && line[0]!= '/')
            {
                line.erase(line.find_last_not_of(" \n\r\t")+1);
                std::transform(line.begin(), line.end(), line.begin(), ::tolower);
                lAllowedProcesses.push_back(line);
                cout << "    " << line << endl;
            }
        }
        myfile.close();
    }
    else
    {
        cout << "Unable to open AllowedProcesses.cfg" << endl << "Press any key to close...";
        getch();
        exit(0);
    }
}

/* -------------------------------------------------------------------------- *
*    Creates a list of running Processes
*  -------------------------------------------------------------------------- */
void GetRunningProcesses()
{
    ProcessData tmpProcessData;
    HANDLE hSnapShot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    PROCESSENTRY32* processInfo=new PROCESSENTRY32;
    processInfo->dwSize=sizeof(PROCESSENTRY32);

    while(Process32Next(hSnapShot,processInfo)!=FALSE)
    {
        tmpProcessData.PID = processInfo->th32ProcessID;
        tmpProcessData.name = processInfo->szExeFile;
        std::transform(tmpProcessData.name.begin(), tmpProcessData.name.end(), tmpProcessData.name.begin(), ::tolower);
        #if DEBUG
        SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text
        cout << "    " << processInfo->th32ProcessID << "    \t" << tmpProcessData.name << endl;
        #endif
        tmpProcessData.killed = false;
        if (tmpProcessData.PID != 0) // Dont add any system prosesses to the list
            vRunningProcesses.push_back(tmpProcessData);
    }
    CloseHandle(hSnapShot);
}

/* -------------------------------------------------------------------------- *
*    Kills Windows
*  -------------------------------------------------------------------------- */
void KillWindows()
{
	char cmdstr[30];
    unsigned int i,j;

    for (i=0; i<vWindowData.size(); i++) // loop thru stored window info
    {
        for (j=0; j<vRunningProcesses.size(); j++) // loop thru stored process info
        {
            if (vRunningProcesses[j].PID == vWindowData[i].PID)
            {
                if (find (lIgnoredPIDs.begin(), lIgnoredPIDs.end(), vRunningProcesses[j].PID) == lIgnoredPIDs.end()) // kill if not on ignored PID list
                {
                    if (find (lAllowedProcesses.begin(), lAllowedProcesses.end(), vRunningProcesses[j].name) == lAllowedProcesses.end()) // kill if not on allowed process name list
                    {
                        SetConsoleTextAttribute(hConsole, RED); // RED text

                        if (close == false){
                            vRunningProcesses[j].killed = true;
                            lIgnoredPIDs.push_back(vRunningProcesses[j].PID); // Ignore thie pid of the process just killed incase it had other windows
                            cout << "Killing Window" << endl;
                        } else {
                            cout << "Closing Window" << endl;
                        }
                        SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text
                        cout << "Title: (" << vWindowData[i].title << ")" << endl
                             << "PID: (" << vRunningProcesses[j].PID << ")"  << endl
                             << "Process name: (" << vRunningProcesses[j].name << ")"  << endl; // print info of window being killed
                        if (test == false){
                            if (close == true){
                                PostMessage(vWindowData[i].hwnd, WM_CLOSE, 0, 0);
                            } else {
                                memset(cmdstr, '\0', sizeof(cmdstr)); // null string to prevent bad output
                                sprintf(cmdstr, "taskkill /PID %u /F", vRunningProcesses[j].PID); // phrase output
                                system(cmdstr); // exicute kill command
                            }
                        }
                        cout << endl;

                    }
                    else
                    {
                        SetConsoleTextAttribute(hConsole, BLUE);
                        cout << "Ignoring Window" << endl;
                        SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text
                        cout << "Title: (" << vWindowData[i].title << ")" << endl
                             << "PID: (" << vRunningProcesses[j].PID << ")"  << endl
                             << "Process name: (" << vRunningProcesses[j].name << ")"  << endl << endl;
                    }
                }
                else
                {
                    if(vRunningProcesses[j].killed)
                    {
                        SetConsoleTextAttribute(hConsole, RED); // RED text
                        cout << "Killed Window" << endl;
                        SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text
                        cout << "Title: (" << vWindowData[i].title << ")" << endl
                             << "PID: (" << vRunningProcesses[j].PID << ")"  << endl
                             << "Process name: (" << vRunningProcesses[j].name << ")"  << endl << endl; // print info of window being killed
                    }
                    else
                    {
                        SetConsoleTextAttribute(hConsole, BLUE);
                        cout << "Ignoring Window" << endl;
                        SetConsoleTextAttribute(hConsole, DEFAULT_COLOR); // regurlar white text
                        cout << "Title: (" << vWindowData[i].title << ")" << endl
                             << "PID: (" << vRunningProcesses[j].PID << ")"  << endl
                             << "Process name: (" << vRunningProcesses[j].name << ")"  << endl << endl;
                    }
                }
                break;
            }
        }
    }
}
