@echo off
strip obj\smmu.exe
cd \doom2
ipxboom -exe smmu\obj\smmu -trideath %1 %2 %3 %4 %5 %6 %7 %8 %9
cd \doom2\smmu
