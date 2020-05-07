#pragma once

CString FindDestinationName(const CString &source);
HKEY GetRegistryKey(HKEY key, const CString &subkey, REGSAM regsam=KEY_ALL_ACCESS );
CString PathToExecutable(const CString &executable);
CString PathToExecutable(const CString &executable, const CString &executablepath);
CString RemovePathFromExecutablePath(const CString &path, const CString &executablepath);

