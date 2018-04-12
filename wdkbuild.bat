echo "Compiling %2 %3 %4... "
call C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\ %2 %3 %4
cd /d %1
build -cgbZ
%5