#include "windowscache.h"
#include <windows.h>

namespace {
QString systemError(const QString &stage, DWORD code)
{
    wchar_t message[512]{};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, message, 512, nullptr);
    return QStringLiteral("%1: %2 (%3)").arg(stage, QString::fromWCharArray(message).trimmed()).arg(code);
}

// Restore privileges to their original state, including on partial failure.
struct Privilege {
    HANDLE token = nullptr;
    TOKEN_PRIVILEGES previous{};
    ~Privilege() {
        if (token) {
            if (previous.PrivilegeCount)
                AdjustTokenPrivileges(token, FALSE, &previous, 0, nullptr, nullptr);
            CloseHandle(token);
        }
    }
    bool enable(const wchar_t *name, QString &error) {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &token)) {
            error = systemError(QStringLiteral("OpenProcessToken"), GetLastError()); return false;
        }
        TOKEN_PRIVILEGES requested{};
        requested.PrivilegeCount = 1;
        if (!LookupPrivilegeValueW(nullptr, name, &requested.Privileges[0].Luid)) {
            error = systemError(QString::fromWCharArray(name), GetLastError()); return false;
        }
        requested.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        DWORD returned = 0;
        SetLastError(ERROR_SUCCESS);
        const BOOL adjusted = AdjustTokenPrivileges(token, FALSE, &requested,
                                                    sizeof(previous), &previous, &returned);
        const DWORD code = GetLastError();
        if (!adjusted || code != ERROR_SUCCESS) {
            error = systemError(QString::fromWCharArray(name), code); return false;
        }
        return true;
    }
};
}

bool WindowsCache::clear(QString &error)
{
    error.clear();
    Privilege profile, quota;
    if (!profile.enable(L"SeProfileSingleProcessPrivilege", error)
            || !quota.enable(L"SeIncreaseQuotaPrivilege", error))
        return false;

    // Native commands used by Windows memory-list utilities. Resolve at runtime:
    // this interface is not a guaranteed public Windows API.
    using SetInformation = LONG (NTAPI *)(ULONG, PVOID, ULONG);
    auto setInformation = reinterpret_cast<SetInformation>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtSetSystemInformation"));
    if (!setInformation) {
        error = systemError(QStringLiteral("NtSetSystemInformation"), ERROR_PROC_NOT_FOUND);
        return false;
    }
    auto command = [&](ULONG operation, const QString &stage) {
        constexpr ULONG systemMemoryListInformation = 80;
        const LONG status = setInformation(systemMemoryListInformation, &operation, sizeof(operation));
        if (status < 0) {
            error = QStringLiteral("%1: NTSTATUS 0x%2").arg(stage)
                    .arg(static_cast<ULONG>(status), 8, 16, QLatin1Char('0'));
            return false;
        }
        return true;
    };
    // Flush dirty pages, trim the system file-cache working set, then discard
    // standby pages. Do not empty other processes' working sets or change limits.
    if (!command(3, QStringLiteral("MemoryFlushModifiedList"))) return false;
    if (!SetSystemFileCacheSize(static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1), 0)) {
        error = systemError(QStringLiteral("SetSystemFileCacheSize"), GetLastError()); return false;
    }
    return command(4, QStringLiteral("MemoryPurgeStandbyList"));
}
