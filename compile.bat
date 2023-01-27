@echo off 

set sdcc_path="C:\Program Files\SDCC"

set project_name=CH554MIDIHOST
rem RX/TX buffer are allocated to 0x00-0x80 (64bytes each) 
rem compiler does not take into account these buffer
rem so adjusting xram_size and xram_loc is needed.
set xram_size=0x0380
set xram_loc=0x0080
set code_size=0x3800
set dfreq_sys=24000000


%sdcc_path%\bin\sdcc -c -mmcs51 --model-small --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I/ -DFREQ_SYS=%dfreq_sys%  main.c
%sdcc_path%\bin\sdcc -c -mmcs51 --model-small --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I/ -DFREQ_SYS=%dfreq_sys%  util.c
%sdcc_path%\bin\sdcc -c -mmcs51 --model-small --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I/ -DFREQ_SYS=%dfreq_sys%  USBHost.c
%sdcc_path%\bin\sdcc -c -mmcs51 --model-small --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I/ -DFREQ_SYS=%dfreq_sys%  uart.c
%sdcc_path%\bin\sdcc -c -mmcs51 --model-small --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I/ -DFREQ_SYS=%dfreq_sys%  midi.c

%sdcc_path%\bin\sdcc main.rel midi.rel util.rel USBHost.rel uart.rel -mmcs51 --model-small --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I/ -DFREQ_SYS=%dfreq_sys%  -o %project_name%.ihx

%sdcc_path%\bin\packihx %project_name%.ihx > %project_name%.hex

del %project_name%.lk
del %project_name%.map
del %project_name%.mem
del %project_name%.ihx

del *.asm
del *.lst
del *.rel
del *.rst
del *.sym