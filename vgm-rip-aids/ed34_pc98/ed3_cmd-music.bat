@echo off
copy ED3_DT%1\%2 ED3_DT10\006
cd ED3_DT10
call packme
cd..
pause
