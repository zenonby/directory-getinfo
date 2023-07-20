# directory-getinfo

Qt5 / VS2022

Ubuntu builds:
* In order to fix an issue with xcb plugin while starting QtCreator (qt.qpa.plugin: Could not load the Qt platform plugin "xcb" in "..." even though it was found.) execute:
	sudo apt install libxcb-xinerama0
* In case of missing libgl (Cannot find -lGL: No such file ot directory) while running the project install corresponding dev package:
	sudo apt install libgl-dev

