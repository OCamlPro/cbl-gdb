#include "pch.h"

#include "walker.h"

#include <winerror.h>

#define EQ ==
#define AND &&
#define OR ||

CString FindDestinationName(const CString &source)
    {
    // We expect that this directory has one, and only one, directory in it
    // Find it.
    CString retval;

    CString root = source;
    root += L"\\*.*";

    WIN32_FIND_DATA  wfd;

    HANDLE hFind = FindFirstFile(root,&wfd);
    BOOL bFound = hFind != INVALID_HANDLE_VALUE ;
    while( bFound )
        {
        if( wcscmp(wfd.cFileName,L".") != 0 AND wcscmp(wfd.cFileName,L"..") != 0  AND (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            {
            // We found ourselves a directory:
            retval = wfd.cFileName;
            break;
            }

        bFound = FindNextFile(hFind,&wfd);
        }

    return retval;
    }

/*  Global Environment Variables are found in 

    Computer\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment

    The current user's Environment Variables are found in

    Computer\HKEY_CURRENT_USER\Environment

*/

HKEY GetRegistryKey(HKEY key, const CString &subkey, REGSAM regsam)
    {
    HKEY retval = 0;
    LSTATUS lstatus = RegOpenKeyEx(key, subkey, 0, regsam, &retval);
    if( lstatus != ERROR_SUCCESS )
        {
        retval = 0;
        }

    return retval;
    }

CString PathToExecutable(const CString &executable, const CString &executablepath_)
    {
    CString executablepath = executablepath_;
    executablepath += ";";
    CString retval;
    for(int i=0;;i++)
        {
        CString path;
        CString path_plus;
        if( !AfxExtractSubString(path,executablepath_,i,';') )
            {
            // Never found the file
            break;
            }
        if( path.Right(1) EQ '\\' )
            {
            path = path.Left( path.GetLength()-1);
            }
        path_plus = path;
        path_plus += "\\";
        path_plus += executable;
        DWORD attributes = GetFileAttributes(path_plus);
        if( attributes != INVALID_FILE_ATTRIBUTES)
            {
            // We found it
            retval = path;
            break;
            }
        }
    return retval;
    }

CString PathToExecutable(const CString &executable)
    {
    CString retval;

    const int bufsize = 16384;
    WCHAR buffer[bufsize];
    DWORD dsize;

    HKEY key_user_environment = GetRegistryKey(HKEY_CURRENT_USER,L"Environment");
    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_user_environment,NULL,L"Path",RRF_RT_ANY,NULL,buffer,&dsize);
    CString user_path = buffer;
    retval = PathToExecutable(executable,user_path);

    if( retval.IsEmpty() )
        {
        HKEY key_global_environment = GetRegistryKey(HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
        dsize = bufsize;
        buffer[0] = '\0';
        RegGetValue(key_user_environment,NULL,L"Path",RRF_RT_ANY,NULL,buffer,&dsize);
        CString global_path = buffer;
        retval = PathToExecutable(executable,global_path);
        }
    return retval;
    }

CString RemovePathFromExecutablePath(const CString &path_, const CString &executablepath_)
    {
    CString retval;
    CString path = path_;

    if( path.Right(1) EQ '\\' )
        {
        path = path.Left( path.GetLength()-1);
        }
    path.MakeLower();
    path.Trim();

    for(int i=0;;i++)
        {
        CString piece;
        if( !AfxExtractSubString(piece,executablepath_,i,';') )
            {
            // We've hit the end
            break;
            }
        if( piece.Right(1) EQ '\\' )
            {
            piece = piece.Left( piece.GetLength()-1);
            }
        piece.Trim();
        CString original_piece = piece;
        piece.MakeLower();
        if( piece.IsEmpty() )
            {
            continue;
            }

        if(piece EQ path)
            {
            continue;
            }
        retval += original_piece;
        retval += ";" ;
        }
    return retval;
    }

