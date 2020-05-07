
// SetupCopyDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "SetupCopy.h"
#include "SetupCopyDlg.h"
#include "afxdialogex.h"
#include <string>
#include <codecvt>
#include "walker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define EQ ==
#define AND &&
#define OR ||

#define MESSAGE_BOX_HEADER L"Symas/Camelian - Environment Configuration for GnuCOBOL"

// CSetupCopyDlg dialog

CSetupCopyDlg::CSetupCopyDlg(CWnd* pParent /*=nullptr*/)
    : CDialog(IDD_SETUPCOPY_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSetupCopyDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSetupCopyDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDOK,&CSetupCopyDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSetupCopyDlg message handlers

BOOL CSetupCopyDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    CString command_line = GetCommandLine();

    AfxExtractSubString(source_directory1,command_line,1,' ');
    AfxExtractSubString(source_directory2,command_line,2,' ');

    CString initial_d = L"c:\\";
    initial_d += source_directory2;

    SetDlgItemText(IDC_DESTINATION,initial_d);

    path_to_original_executable = PathToExecutable(L"cobc.exe");

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSetupCopyDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSetupCopyDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

std::string ws2s(const std::wstring& wstr)
    {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
    }

void ExecuteCommand(CString &command_line)
    {
    STARTUPINFO startup_info;
    memset(&startup_info,0,sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info;
    memset(&process_info,0,sizeof(process_info));

    LPWSTR lpc = (LPWSTR)command_line.GetString();

    bool process_okay = CreateProcess
    (
        NULL, //iscmd ? L"cmd.exe" : NULL,
        lpc,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &startup_info,
        &process_info
    );

    if( process_okay )
        {
        WaitForSingleObject( process_info.hProcess, INFINITE );
        CloseHandle( process_info.hProcess );
        CloseHandle( process_info.hThread );
        }
    }

void CSetupCopyDlg::CheckEnvironment(const CString &destination_)
    {
    CString destination = destination_;
    destination.MakeLower();

    const CString PATH=L"Path";
    const CString COB_LIBRARY_PATH=L"COB_LIBRARY_PATH";
    const CString COB_CONFIG_DIR=L"COB_CONFIG_DIR";
    const CString COB_COPY_DIR=L"COB_COPY_DIR";

    CString path_to_bin = destination;
    CString library_path = destination;
    CString config_dir = destination;
    CString copy_dir = destination;

    path_to_bin += L"\\bin";
    library_path += L"\\lib\\gnucobol";
    config_dir += L"\\share\\gnucobol\\config";
    copy_dir += L"\\share\\gnucobol\\copy";

    HKEY key_user_environment = GetRegistryKey(HKEY_CURRENT_USER,L"Environment");
    HKEY key_global_environment = GetRegistryKey(HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");

    const int bufsize = 16384;
    WCHAR buffer[bufsize];
    DWORD dsize;

    CString user_path;
    CString user_library;
    CString user_config;
    CString user_copy;
    CString global_path;
    CString global_library;
    CString global_config;
    CString global_copy;

    // Pick up the user variables
    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_user_environment,NULL,PATH,RRF_RT_ANY,NULL,buffer,&dsize);
    user_path = buffer;

    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_user_environment,NULL,COB_LIBRARY_PATH,RRF_RT_ANY,NULL,buffer,&dsize);
    user_library = buffer;

    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_user_environment,NULL,COB_CONFIG_DIR,RRF_RT_ANY,NULL,buffer,&dsize);
    user_config = buffer;

    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_user_environment,NULL,COB_COPY_DIR,RRF_RT_ANY,NULL,buffer,&dsize);
    user_copy = buffer;

    // And the global variables
    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_global_environment,NULL,PATH,RRF_RT_ANY,NULL,buffer,&dsize);
    global_path = buffer;

    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_global_environment,NULL,COB_LIBRARY_PATH,RRF_RT_ANY,NULL,buffer,&dsize);
    global_library = buffer;

    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_global_environment,NULL,COB_CONFIG_DIR,RRF_RT_ANY,NULL,buffer,&dsize);
    global_config = buffer;

    dsize = bufsize;
    buffer[0] = '\0';
    RegGetValue(key_global_environment,NULL,COB_COPY_DIR,RRF_RT_ANY,NULL,buffer,&dsize);
    global_copy = buffer;

    CString user_path_lower = user_path;
    user_path_lower  .MakeLower();
    global_path      .MakeLower();

    user_library     .MakeLower();
    user_config      .MakeLower();
    user_copy        .MakeLower();

    global_library   .MakeLower();
    global_config    .MakeLower();
    global_copy      .MakeLower();

    BOOL found_in_user_path = user_path_lower.Find(path_to_bin) >= 0 ? TRUE : FALSE;
    BOOL found_in_global_path = global_path.Find(path_to_bin) >= 0 ? TRUE : FALSE;
    
    if(     !found_in_user_path
       AND  !found_in_global_path
       AND   user_library   .IsEmpty()
       AND   user_config    .IsEmpty()
       AND   user_copy      .IsEmpty()
       AND   global_library .IsEmpty()
       AND   global_config  .IsEmpty()
       AND   global_copy    .IsEmpty() )
        {
        // This is Case Zero:  
        CString message = L"";
        message += L"You have a choice to make.\n";
        message += L"\n";
        message += L"In order to use the GnuCOBOL compiler, the environment has to be set up\n";
        message += L"properly.\n";
        message += L"\n";
        message += L"You can do this manually by opening a Command Prompt window and executing\n";
        message += L"\n";
        message += L"         ";
        message += destination_;
        message += L"\\bin\\cobenv --setenv\n";
        message += L"\n";
        message += L"If you prefer to be able to use GnuCOBOL from any Command Prompt, we can put ";
        message += path_to_bin;
        message += L" into your execution path, and permanently add ";
        message += L"these necessary variables to your environment:\n";
        message += L"\n";
        message += L"         ";
        message += COB_LIBRARY_PATH;
        message += L"=";
        message += library_path;
        message += L"\n";

        message += L"         ";
        message += COB_CONFIG_DIR;
        message += L"=";
        message += config_dir;
        message += L"\n";

        message += L"         ";
        message += COB_COPY_DIR;
        message += L"=";
        message += copy_dir;
        message += L"\n";

        message += L"\n";
        message += L"Would you like us to make those changes?\n";
        if( MessageBox(message,MESSAGE_BOX_HEADER,MB_YESNO) EQ IDYES)
            {
            //  Full environment setup:
            CString newpath = path_to_bin;
            newpath += ":";
            newpath += user_path;

            RegSetValueEx(key_user_environment,
                          PATH,
                          0,
                          REG_EXPAND_SZ,
                          (const BYTE *)newpath.GetString(),
                          2*(newpath.GetLength()+1)
                          );
            RegSetValueEx(key_user_environment,
                          COB_LIBRARY_PATH,
                          0,
                          REG_EXPAND_SZ,
                          (const BYTE *)library_path.GetString(),
                          2*(library_path.GetLength()+1)
                          );
            RegSetValueEx(key_user_environment,
                          COB_CONFIG_DIR,
                          0,
                          REG_EXPAND_SZ,
                          (const BYTE *)config_dir.GetString(),
                          2*(config_dir.GetLength()+1)
                          );
            RegSetValueEx(key_user_environment,
                          COB_COPY_DIR,
                          0,
                          REG_EXPAND_SZ,
                          (const BYTE *)copy_dir.GetString(),
                          2*(copy_dir.GetLength()+1)
                          );
            message  = L"The user environment has been set up for GnuCOBOL.\n";
            message += L"\n";
            message += L"This completes the installation";
            MessageBox(message,MESSAGE_BOX_HEADER,MB_OK); 
            return;
            }
        else
            {
            message  = L"Very good. To use the compiler, open a Command Prompt and execute\n";
            message += L"\n";
            message += L"         ";
            message += destination_;
            message += L"\\bin\\cobenv --setenv\n";
            message += L"\n";
            message += L"That command is mentioned in the ";
            message += destination_;
            message += L"\\README.txt file.\n";
            message += L"\n";
            message += L"This completes the installation";
            MessageBox(message,MESSAGE_BOX_HEADER,MB_OK); 
            return;
            }
        }

    // Arriving here means that at least some of those variables already
    // are in the registry.
    if(    (found_in_user_path OR found_in_global_path)  
       AND ( (!user_library.IsEmpty() AND user_library EQ library_path) OR (global_library EQ library_path) )
       AND ( (!user_config.IsEmpty()  AND user_config  EQ config_dir)   OR (global_config  EQ config_dir) )
       AND ( (!user_copy.IsEmpty()    AND user_copy    EQ copy_dir)     OR (global_copy    EQ copy_dir) )
        )
        {
        CString message;
        message  = L"We see that your environment is already set up to use GnuCOBOL's files at \n";
        message += L"\n";
        message += L"       ";
        message += destination_;
        message += L"\n";
        message += L"\n";
        message += L"This completes the installation";
        MessageBox(message,MESSAGE_BOX_HEADER,MB_OK); 
        return;
        }

    // Arriving here means that the environment isn't right:

    CString message;
    message += L"You have a choice to make.\n";
    message += L"\n";
    message += "Your existing environment isn't correctly set up to run GnuCOBOL from\n";
    message += L"\n";
    message += L"       ";
    message += destination_;
    message += L"\n";
    message += L"\n";
    message += "The current environment is:\n";
    message += L"\n";

    if( !path_to_original_executable.IsEmpty() )
        {
        message += L"       ";
        message += path_to_original_executable;
        message += L"\\cobc.exe is the current executable ";
        message += path_to_original_executable EQ path_to_bin ? L"(okay)" : L"(no good)";
        message += L"\n";
        }

    if( !found_in_user_path AND !found_in_user_path)
        {
        message += L"       ";
        message += destination_;
        message += L"\\bin isn't on the execution path (no good)\n";
        }
    else
        {
        message += L"       ";
        message += destination_;
        message += L"\\bin is on the execution path (okay)\n";
        }
    CString temp;

    temp = (!user_library.IsEmpty()) ? user_library : global_library;
    message += L"       ";
    message += COB_LIBRARY_PATH;
    message += L"=";
    message += temp;
    message += L" ";
    message += temp EQ library_path ? L"(okay)" : L"(no good)";
    message += L"\n";

    temp = (!user_config.IsEmpty()) ? user_config : global_config;
    message += L"       ";
    message += COB_CONFIG_DIR;
    message += L"=";
    message += temp;
    message += L" ";
    message += temp EQ config_dir ? L"(okay)" : L"(no good)";
    message += L"\n";

    temp = (!user_copy.IsEmpty()) ? user_copy : global_copy;
    message += L"       ";
    message += COB_COPY_DIR;
    message += L"=";
    message += temp;
    message += L" ";
    message += temp EQ copy_dir ? L"(okay)" : L"(no good)";
    message += L"\n";
    message += L"\n";

    message += L"You can leave this environment alone, which means that using GnuCOBOL requires opening a Command Prompt window and executing\n";
    message += L"\n";
    message += L"         ";
    message += destination_;
    message += L"\\bin\\cobenv --setenv\n";
    message += L"\n";
    message += L"Or you can let us change your environment so that you can use GnuCOBOL from any Command Prompt window.\n";
    message += L"\n";
    message += L"Would you like us to make those changes?\n";

    if( MessageBox(message,MESSAGE_BOX_HEADER,MB_YESNO) EQ IDYES)
        {
        // If somebody (like Dubner, during development and test) keeps putting
        // in differently named folders, the path could get nuts; we could end up
        // with PATH=A;B;A;B;A;B;....
        
        // Let's avoid that silliness removing what's probably the existing path:

        if( !path_to_original_executable.IsEmpty() )
            {
            user_path = RemovePathFromExecutablePath(path_to_original_executable,user_path);
            RegSetValueEx(key_user_environment,
                          PATH,
                          0,
                          REG_EXPAND_SZ,
                          (const BYTE *)user_path.GetString(),
                          2*(user_path.GetLength()+1)
                          );
            }

        user_path_lower = user_path;
        user_path_lower.MakeLower();
        found_in_user_path = user_path_lower.Find(path_to_bin) >= 0 ? TRUE : FALSE;

        if( !found_in_user_path )
            {
            CString newpath = path_to_bin;
            newpath += ";";
            newpath += user_path;

            RegSetValueEx(key_user_environment,
                          PATH,
                          0,
                          REG_EXPAND_SZ,
                          (const BYTE *)newpath.GetString(),
                          2*(newpath.GetLength()+1)
                          );
            }
        RegSetValueEx(key_user_environment,
                      COB_LIBRARY_PATH,
                      0,
                      REG_EXPAND_SZ,
                      (const BYTE *)library_path.GetString(),
                      2*(library_path.GetLength()+1)
                      );
        RegSetValueEx(key_user_environment,
                      COB_CONFIG_DIR,
                      0,
                      REG_EXPAND_SZ,
                      (const BYTE *)config_dir.GetString(),
                      2*(config_dir.GetLength()+1)
                      );
        RegSetValueEx(key_user_environment,
                      COB_COPY_DIR,
                      0,
                      REG_EXPAND_SZ,
                      (const BYTE *)copy_dir.GetString(),
                      2*(copy_dir.GetLength()+1)
                      );
        message  = L"The user environment has been set up for GnuCOBOL.\n";
        message += L"\n";
        message += L"This completes the installation";
        MessageBox(message,MESSAGE_BOX_HEADER,MB_OK); 
        return;
        }
    else
        {
        message  = L"Very good. To use the compiler, open a Command Prompt and execute\n";
        message += L"\n";
        message += L"         ";
        message += destination_;
        message += L"\\bin\\cobenv --setenv\n";
        message += L"\n";
        message += L"That command is mentioned in the ";
        message += destination_;
        message += L"\\README.txt file.\n";
        message += L"\n";
        message += L"This completes the installation";
        MessageBox(message,MESSAGE_BOX_HEADER,MB_OK); 
        return;
        }

    }

void CSetupCopyDlg::OnBnClickedOk()
    {
    // Pick up the installation destination
    CString destination;
    GetDlgItemText(IDC_DESTINATION,destination);

    BOOL proceed = TRUE;
    if( PathFileExists(destination) )
        {
        CString message;
        message = destination;
        message += " already exists\n\nShall we replace it?";
        if( MessageBox(message,L"Directory Already Exists",MB_ICONWARNING | MB_YESNO) == IDYES )
            {
            CString command_line;

            command_line = "cmd.exe /c rmdir /S /Q  ";
            command_line += destination;
            CString at_first;
            GetDlgItemText(IDC_STATICMSG,at_first);
            SetDlgItemText(IDC_STATICMSG,L"Deleting...");
            ExecuteCommand(command_line);
            SetDlgItemText(IDC_STATICMSG,at_first);
            }
        else
            {
            proceed = FALSE;
            }
        }

    if( proceed )
        {
        CString command_line;

        command_line = "xcopy ";
        command_line += source_directory1;
        command_line += "\\";
        command_line += source_directory2;
        command_line += "\\*.* ";
        command_line += destination;
        command_line += " /S /E /H /I /Q /K /B";

        //MessageBox(command_line);
        SetDlgItemText(IDC_STATICMSG,L"Copying...");
        ExecuteCommand(command_line);

        SetDlgItemText(IDC_STATICMSG,L"Checking environment...");
        CheckEnvironment(destination);

        }
    CDialog::OnOK();
    }
