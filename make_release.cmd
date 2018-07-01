@echo off
set PROJECT=text_clipboard
set VERSION=1.2.0

set RELEASE_DIR=.\build\%PROJECT%_%VERSION%

git diff-index --quiet HEAD --
if errorlevel 1 (
    echo Uncommited changes detected, commit first
    exit /b 1
)
git tag --annotate -m "" v%VERSION% master
if errorlevel 1 exit /b 1
if exist .\build (
    rmdir /S /Q .\build
    if errorlevel 1 exit /b 1
)

msbuild %PROJECT%.sln /p:Configuration=Release;Platform=x86;ProjectVersion=%VERSION%
if errorlevel 1 exit /b 1
msbuild %PROJECT%.sln /p:Configuration=Release;Platform=x64;ProjectVersion=%VERSION%
if errorlevel 1 exit /b 1

powershell.exe -file "%~dp0github_sourceindexer.ps1" ^
	-symbolsFolder .\build ^
	-userId sekogan ^
	-repository TextClipboard ^
	-branch v%VERSION% ^
	-ignoreUnknown ^
	-verbose

mkdir %RELEASE_DIR%
7z a %RELEASE_DIR%\%PROJECT%_32_%VERSION%.zip .\build\Release_Win32\text_clipboard.exe .\scripts\*
7z a %RELEASE_DIR%\%PROJECT%_32_%VERSION%_pdb.zip .\build\Release_Win32\text_clipboard.pdb
7z a %RELEASE_DIR%\%PROJECT%_64_%VERSION%.zip .\build\Release_x64\text_clipboard.exe .\scripts\*
7z a %RELEASE_DIR%\%PROJECT%_64_%VERSION%_pdb.zip .\build\Release_x64\text_clipboard.pdb
