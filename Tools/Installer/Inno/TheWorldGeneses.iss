; Instalador do The World Gêneses (Inno Setup 6). Gerado pelo Tools\PackageAlpha.ps1, que passa:
;   /DSourceDir=<pasta do jogo empacotado>  /DOutputDir=<onde salvar>  /DAppVersion=1.0.0-alpha
#ifndef SourceDir
  #define SourceDir "..\..\..\Build\Alpha\Windows"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\..\Build\Installer"
#endif
#ifndef AppVersion
  #define AppVersion "1.0.0-alpha"
#endif
#define AppName "The World Gêneses"
#define AppExe "Vortex.exe"

[Setup]
AppId={{6E2B9C1A-4F7D-4B8E-9A31-5C30205D4869}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} Alpha 1.0
AppPublisher=Eros Nicolino
DefaultDirName={autopf}\The World Geneses
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} (Alpha 1.0)
UninstallDisplayIcon={app}\Vortex.ico
OutputDir={#OutputDir}
OutputBaseFilename=TheWorldGeneses_Alpha_1.0_Setup
SetupIconFile={#SourceDir}\Vortex.ico
WizardImageFile=wizard.bmp
WizardSmallImageFile=wizard_small.bmp
WizardStyle=modern dynamic
WizardImageStretch=yes
WizardSizePercent=100
SetupLogging=yes
VersionInfoCompany=Eros Nicolino
VersionInfoDescription={#AppName} Alpha 1.0 Installer
VersionInfoProductName={#AppName}
VersionInfoProductVersion=1.0.0.0
; os .pak/.utoc já vêm comprimidos (Oodle): compressão leve basta e não estoura a memória
Compression=lzma2/fast
SolidCompression=no
LZMAUseSeparateProcess=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequiredOverridesAllowed=dialog
DisableProgramGroupPage=yes

[Languages]
Name: "ptbr"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Messages]
WelcomeLabel1=Bem-vindo a Valdora
WelcomeLabel2=Este assistente prepara The World Gêneses no seu computador.%n%nCrie seu herói, explore um mundo fantástico e descubra a história por trás do Vórtice.
FinishedHeadingLabel=Sua jornada começa agora
FinishedLabel=The World Gêneses foi instalado.%n%nClique em Jogar agora para despertar em Valdora.

[Tasks]
Name: "desktopicon"; Description: "Criar um atalho na área de trabalho"; GroupDescription: "Atalhos:"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "Manifest_*.txt,*.pdb,Vortex.ico"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\Vortex.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"; IconFilename: "{app}\Vortex.ico"
Name: "{group}\Desinstalar {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; IconFilename: "{app}\Vortex.ico"; Tasks: desktopicon

[Run]
; bibliotecas do Visual C++ e do DirectX que a Unreal precisa (silencioso; já instaladas = nada acontece)
Filename: "{app}\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe"; Parameters: "/quiet /norestart"; StatusMsg: "Instalando os componentes do Windows que o jogo precisa..."; Check: FileExists(ExpandConstant('{app}\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe')); Flags: waituntilterminated
Filename: "{app}\{#AppExe}"; Description: "Jogar agora"; Flags: nowait postinstall skipifsilent
