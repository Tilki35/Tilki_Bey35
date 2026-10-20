#include "localization.hpp"
// A lightweight GUI launcher, not a bundled Python interpreter. The package
// includes an installer and source UI; a target-Windows frozen build is separate.
int WINAPI wWinMain(HINSTANCE,HINSTANCE,LPWSTR,int) {
    try {
        auto folder=executableDirectory("");LocaleArguments args;
        Localizer locale(args,folder);activeLocalizer=&locale;
        std::filesystem::path interpreter=folder/L".venv/Scripts/pythonw.exe";
        std::wstring arguments=L"-m desktop.app";
        if(!std::filesystem::exists(interpreter)) {
            wchar_t path[32768]{};
            if(SearchPathW(nullptr,L"pythonw.exe",nullptr,32768,path,nullptr))interpreter=path;
            else if(SearchPathW(nullptr,L"pyw.exe",nullptr,32768,path,nullptr)){interpreter=path;arguments=L"-3 -m desktop.app";}
            else {MessageBoxW(nullptr,wide(locale.text("desktop.error.python_required")).c_str(),wide(locale.name()).c_str(),MB_OK|MB_ICONINFORMATION);return 1;}
        }
        auto result=ShellExecuteW(nullptr,L"open",interpreter.c_str(),arguments.c_str(),folder.c_str(),SW_SHOWNORMAL);
        if(reinterpret_cast<INT_PTR>(result)<=32)throw std::runtime_error("desktop.error.python_required");
        return 0;
    } catch(const std::exception&) {MessageBoxW(nullptr,L"Run Install-desktop.cmd first.",L"EVRKare / UniFrame",MB_OK|MB_ICONERROR);return 1;}
}
