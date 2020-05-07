:: Use this .cmd script to create a self-extracting executable that
:: runs setup.exe after finishing the decompression into a temp
:: directory
::
:: $1 parameter is the folder to be packaged up
:: $2 parameter is the file name of the self-extracting executable
::

@echo ;!@Install@!UTF-8!                      >config.txt
@echo Title="Camelian software installation" >>config.txt
@echo RunProgram="setup.exe %%%%T %1"          >>config.txt
@echo ;!@InstallEnd@!                        >>config.txt

erase %1.7z
erase %2.exe
"c:\Program Files\7-zip\7z" a -mmt12 -mx9 -r -y %1.7z %1 setup.exe
copy /b 7zSD-admin.sfx + config.txt + %1.7z %2.exe

