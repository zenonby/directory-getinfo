project(sqlite3
    VERSION   3.42.0
    LANGUAGES C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
	amalgamation/sqlite3.c
	amalgamation/sqlite3.h
	SqliteDb.cpp
	SqliteDb.h
	SqliteRecordset.cpp
	SqliteRecordset.h
	SqliteCommand.cpp
	SqliteCommand.h
	SqliteTransaction.cpp
	SqliteTransaction.h
)

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME   sqlite3
    PUBLIC_HEADER SqliteDb.h
    DEBUG_POSTFIX d
)
