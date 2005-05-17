; -- wxCas.iss --
; Windows installation of wxCas

[Setup]
AppName=wxCas
AppVersion=CVS
AppVerName=wxCas CVS
AppComments=wxCas is part of aMule project
AppPublisher=www.amule.org
DefaultDirName={pf}\amule\wxCas
DefaultGroupName=aMule
UninstallDisplayIcon={app}\wxcas.exe
Compression=lzma
SolidCompression=yes
LicenseFile="..\..\..\docs\license.txt"

[Files]
Source: "Release\wxcas.exe"; DestDir: "{app}"
Source: "..\..\..\docs\license.txt"; DestDir: "{app}"
Source: "C:\MinGWStudio\MinGW\bin\mingwm10.dll"; DestDir: "{app}"

[Icons]
Name: "{group}\wxCas"; Filename: "{app}\wxcas.exe"

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
Filename: "{app}\wxcas.exe"; Flags: postinstall nowait skipifsilent

