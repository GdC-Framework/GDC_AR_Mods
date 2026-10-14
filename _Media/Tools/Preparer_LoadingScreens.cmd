@echo off
rem Crop and resize the new loading screens of _Media\LoadingScreens\_Originaux
rem into _Media\LoadingScreens\Exports (double-click, or pass --force to redo all).
python -c "import PIL" 2>nul || python -m pip install --user pillow
python "%~dp0prepare_loading_screens.py" %*
pause
