FROM mcr.microsoft.com/windows/servercore:ltsc2022 AS runtime

WORKDIR C:\nexusflow

COPY build\windows-release\Release\nexusflow.exe C:\nexusflow\nexusflow.exe

ENTRYPOINT ["C:\\nexusflow\\nexusflow.exe"]
