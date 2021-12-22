del *.o /s
del *.elf
del *.dol
del *.map
make -f Makefile.glN64_gc --jobs=5
del *.o /s
make -f Makefile.glN64_gc_exp --jobs=5
del *.o /s
make -f Makefile.glN64_gc_basic --jobs=5
del *.o /s
make -f Makefile.glN64_wii --jobs=5
del *.o /s
make -f Makefile.glN64_wiivc --jobs=5
del *.o /s
make -f Makefile.Rice_gc --jobs=5
del *.o /s
make -f Makefile.Rice_gc_exp --jobs=5
del *.o /s
make -f Makefile.Rice_gc_basic --jobs=5
del *.o /s
make -f Makefile.Rice_wii --jobs=5
del *.o /s
make -f Makefile.Rice_wiivc --jobs=5
del *.o /s

del release\*.dol /s
mv cube*.dol release\gamecube\
mv wii64-glN64.dol release\apps\wii64\boot.dol
mv wii64-glN64-wiivc.dol release\apps\wii64_wiivc\boot.dol
mv wii64-Rice.dol "release\apps\wii64 Rice\boot.dol"
mv wii64-Rice-wiivc.dol "release\apps\wii64 Rice_wiivc\boot.dol"
pause