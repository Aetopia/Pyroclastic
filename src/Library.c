#define _MINAPPMODEL_H_
#include <windows.h>
#include <appmodel.h>
#include <winternl.h>

__declspec(dllexport) VOID GameLaunch(VOID)
{
    Sleep(INFINITE);
}

DWORD ThreadProc(PVOID pParameter)
{
    WCHAR szCurrentPackageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH + 1] = {};
    if (GetCurrentPackageFamilyName(&(UINT){ARRAYSIZE(szCurrentPackageFamilyName)}, szCurrentPackageFamilyName))
        goto _;

    WCHAR szPathName[MAX_PATH] = {};
    if (GetCurrentPackagePath(&(UINT){ARRAYSIZE(szPathName)}, szPathName) || !SetCurrentDirectoryW(szPathName))
        goto _;

    HANDLE hMutex = CreateMutexW(NULL, FALSE, szCurrentPackageFamilyName);

    if (hMutex && !GetLastError())
    {
        PRTL_USER_PROCESS_PARAMETERS pParameters = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters;
        PWSTR pCommandLine = pParameters->CommandLine.Buffer + lstrlenW(pParameters->ImagePathName.Buffer) + 2;

        PROCESS_INFORMATION _ = {};
        CreateProcessW(L"Minecraft.Windows.exe", pCommandLine, NULL, NULL, FALSE, 0, 0, NULL, &(STARTUPINFOW){}, &_);
        CloseHandle(_.hThread);

        HANDLE hObject = {};
        DuplicateHandle(GetCurrentProcess(), hMutex, _.hProcess, &hObject, 0, FALSE, DUPLICATE_SAME_ACCESS);
        CloseHandle(hObject);

        WaitForInputIdle(_.hProcess, INFINITE);
        CloseHandle(_.hProcess);
    }

    CloseHandle(hMutex);

    HWND hWnd = {};
    while ((hWnd = FindWindowExW(NULL, hWnd, L"Bedrock", NULL)))
    {
        DWORD dwProcessId = {};
        GetWindowThreadProcessId(hWnd, &dwProcessId);

        WCHAR szPackageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH + 1] = {};
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);

        if (!GetPackageFamilyName(hProcess, &(UINT32){ARRAYSIZE(szPackageFamilyName)}, szPackageFamilyName) &&
            CompareStringOrdinal(szCurrentPackageFamilyName, -1, szPackageFamilyName, -1, TRUE) == CSTR_EQUAL)
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