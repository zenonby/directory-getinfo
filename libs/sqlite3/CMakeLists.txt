project(sqlite3
    VERSION   3.42.0
    LANGUAGES C
)

add_library(${PROJECT_NAME} STATIC sqlite3.c)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME   sqlite3
    PUBLIC_HEADER sqlite3.h
    DEBUG_POSTFIX d
)
