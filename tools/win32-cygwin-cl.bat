@c:\cygwin\bin\bash.exe -c "pwd | /usr/bin/sed \"y/\\\\//\\\\\\\\/;s/\\\\\\\\cygdrive\\\\\\\\\\\\(.\\\\)\\\\(\\\\\\\\\\\\(.*\\\\)\\\\)\\\\?/[win32] Entering dir \\'\\\\1:\\\\\\\\\\\\3\\'/\""
@"%MSVCDir%\bin\cl.exe" %*
@echo [win32] Leaving dir
