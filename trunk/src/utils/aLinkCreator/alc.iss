; -- alc.iss --
; Windows installation of alc

[Setup]
AppName=alc
AppVersion=CVS
AppVerName=alc CVS
AppComments=alc is part of aMule project
AppPublisher=www.amule.org
DefaultDirName={pf}\amule\alc
DefaultGroupName=aMule
UninstallDisplayIcon={app}\alc.exe
Compression=lzma
SolidCompression=yes
LicenseFile="..\..\..\docs\license.txt"

[Files]
Source: "Release\alc.exe"; DestDir: "{app}"
Source: "..\..\..\docs\license.txt"; DestDir: "{app}"
Source: "C:\MinGWStudio\MinGW\bin\mingwm10.dll"; DestDir: "{app}"

[Icons]
Name: "{group}\alc"; Filename: "{app}\alc.exe"

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "ca"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "cs"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "pt"; MessagesFile: "compiler:Languages\PortugueseStd.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "no"; MessagesFile: "compiler:Languages\Norwegian.isl"

[Run]
Filename: "{app}\alc.exe"; Flags: postinstall nowait skipifsilent

