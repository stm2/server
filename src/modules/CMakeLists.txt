PROJECT(modules C)
SET(_FILES
arena.c
autoseed.c
gmcmd.c
museum.c
score.c
weather.c
xmas.c
)
FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(MODULES_SRC ${_SOURCES} PARENT_SCOPE)
