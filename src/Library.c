#define _MINAPPMODEL_H_
#include <windows.h>
#include <appmodel.h>
#include <winternl.h>
#include <tlhelp32.h>

__declspec(dllexport) VOID GameLaunch(VOID)
{
    Sleep(INFINITE);
}

DWORD ThreadProc(PVOID pParameter)
{
    WCHAR szCurrent[PACKAGE_FAMILY_NAME_MAX_LENGTH + 1] = {};
    if (GetCurrentPackageFamilyName(&(UINT){ARRAYSIZE(szCurrent)}, szCurrent))
        goto _;

    WCHAR szPath[MAX_PATH] = {};
    if (GetCurrentPackagePath(&(UINT){ARRAYSIZE(szPath)}, szPath) || !SetCurrentDirectoryW(szPath))
        goto _;

    PROCESSENTRY32W _ = {.dwSize = sizeof _};
    HANDLE hProcess = {}, hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32NextW(hSnapshot, &_))
        do
            if (CompareStringOrdinal(L"Minecraft.Windows.exe", -1, _.szExeFile, -1, TRUE) == CSTR_EQUAL)
            {
                hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, _.th32ProcessID);
                break;
            }
        while (Process32NextW(hSnapshot, &_));

    if (WaitForInputIdle(hProcess, INFINITE))
    {
        PRTL_USER_PROCESS_PARAMETERS pParameters = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters;
        PWSTR pCommandLine = pParameters->CommandLine.Buffer + lstrlenW(pParameters->ImagePathName.Buffer) + 2;

        PROCESS_INFORMATION _ = {};
        CreateProcessW(L"Minecraft.Windows.exe", pCommandLine, NULL, NULL, FALSE, 0, 0, NULL, &(STARTUPINFOW){}, &_);
        WaitForInputIdle(_.hProcess, INFINITE);

        CloseHandle(_.hThread);
        CloseHandle(_.hProcess);
    }

    CloseHandle(hProcess);
    CloseHandle(hSnapshot);

    HWND hWnd = {};
    while ((hWnd = FindWindowExW(NULL, hWnd, L"Bedrock", NULL)))
    {
        DWORD dwProcessId = {};
        GetWindowThreadProcessId(hWnd, &dwProcessId);

        WCHAR szProcess[PACKAGE_FAMILY_NAME_MAX_LENGTH + 1] = {};
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);

        if (!GetPackageFamilyName(hProcess, &(UINT32){ARRAYSIZE(szProcess)}, szProcess) &&
            CompareStringOrdinal(szCurrent, -1, szProcess, -1, TRUE) == CSTR_EQUAL)
            SwitchToThisWindow(hWnd, TRUE);

        CloseHandle(hProcess);
    }
_:
    return TerminateProcess(GetCurrentProcess(), EXIT_SUCCESS);
}

BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, PVOID pReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        QueueUserWorkItem(ThreadProc, NULL, WT_EXECUTEDEFAULT);
    }
    return TRUE;
}