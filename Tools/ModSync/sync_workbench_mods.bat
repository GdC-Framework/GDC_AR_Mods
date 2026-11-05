@echo off
rem Double-click: shows the plan, asks before copying.
python -I "%~dp0sync_workbench_mods.py" %*
pause
