﻿# directory-getinfo
## Description
Qt-based GUI program for getting information about directories, specifically how much space is occupied by each file type within those directories.

## Submodules
Before building the code checkout submodules:
```
git submodule init
git submodule update
```

## Build system:
* VS2022 - cmake
* Qt6 - qmake (N.B. currently support is on hold!)

## Ubuntu builds:
* In order to fix an issue with xcb plugin while starting QtCreator (_qt.qpa.plugin: Could not load the Qt platform plugin "xcb" in "..." even though it was found._) execute:
```
	sudo apt install libxcb-xinerama0
```
* In case of missing libgl (_Cannot find -lGL: No such file ot directory_) while running the project, install corresponding dev package:
```
	sudo apt install libgl-dev
```

