@echo off
cd \doom2
copy smmu\obj\smmu.exe /y
ipxboom -exe smmu -nodes 1 -trideath %1 %2 %3 %4 %5 %6 %7 %8 %9
cd \doom2\smmu
