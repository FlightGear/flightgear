
add_definitions(-DSQLITE_OMIT_LOAD_EXTENSION)
add_definitions(-DNDEBUG)
add_library(fgsqlite3 sqlite3.c)
set_target_properties(fgsqlite3 PROPERTIES COMPILE_FLAGS "-fpic -fno-fast-math")
