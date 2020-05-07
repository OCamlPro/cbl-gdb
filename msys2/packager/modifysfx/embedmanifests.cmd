::
:: From Visual Studio 2019:  
::
::           Tools -> Command Line -> Developer Command Prompt
::
:: Navigate to this folder, and then execute this script
::
copy 7zSD.sfx 7zSD-admin.sfx
copy 7zSD.sfx 7zSD-invoker.sfx

mt.exe  -manifest "manifest.admin.txt" -outputresource:"7zSD-admin.sfx";#1
mt.exe  -manifest "manifest.invoker.txt" -outputresource:"7zSD-invoker.sfx";#1
