# Ambiente de build Android em VM Windows

## Requisitos validados

- Windows 11 64-bit
- Android Studio com JDK 17 ou mais recente
- Android SDK Platform 34
- Android SDK Build-Tools 34.0.0
- Android SDK Platform-Tools
- CMake 3.22.1
- Android NDK 25.1.8937393
- 8 GB de RAM no mínimo; 12 a 16 GB são preferíveis
- aproximadamente 30 GB livres para o projeto, SDK, emulador e caches

Instale os pacotes pelo Android Studio em:

`Settings > Languages & Frameworks > Android SDK > SDK Tools`

Ative `Show Package Details` para escolher exatamente CMake 3.22.1 e NDK
25.1.8937393.

## Compilar

Abra o PowerShell na pasta `android`.

Emulador x86_64:

```powershell
.\build_android.ps1 -Target emulator
```

Aparelho Android arm64:

```powershell
.\build_android.ps1 -Target device
```

Os APKs são gerados em:

- `app\build\outputs\apk\emulator\debug\app-emulator-debug.apk`
- `app\build\outputs\apk\realDevice\debug\app-realDevice-debug.apk`

## Executar com isolamento

Use uma VM sem pastas compartilhadas, sem área de transferência compartilhada e
sem credenciais pessoais. Tire um snapshot antes da instalação.

O aplicativo baixa automaticamente:

`http://update.daybreak.id.vn/update/data.zip`

Esse download usa HTTP sem TLS, aceita redirecionamentos e contém credenciais
Basic embutidas no APK. Não trate o conteúdo baixado como confiável. Para o
primeiro teste, use uma rede NAT isolada e capture o tráfego. Não execute o
servidor Windows incluído no pacote na máquina principal.

Com um emulador já iniciado:

```powershell
adb install -r .\app\build\outputs\apk\emulator\debug\app-emulator-debug.apk
adb logcat -c
adb shell am start -n com.muonline.client/.PreloadActivity
adb logcat
```

Para impedir o download inseguro, bloqueie a rede do aplicativo e pré-carregue
uma pasta `Data` válida em:

`/sdcard/Android/data/com.muonline.client/files/Data`

## Limpar caches antigos

O pacote original contém caches CMake com caminhos absolutos da máquina do
autor. Se o Gradle voltar a mencionar `D:\takumi`, feche o Android Studio e
remova somente:

`android\app\.cxx`

Depois execute novamente `build_android.ps1`.
