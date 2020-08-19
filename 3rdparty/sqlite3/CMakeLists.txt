
add_library(fgsqlite3 STATIC sqlite3.c)

target_compile_definitions(fgsqlite3 PRIVATE SQLITE_OMIT_LOAD_EXTENSION NDEBUG)
set_target_properties(fgsqlite3 PROPERTIES COMPILE_FLAGS "-fPIC -fno-fast-math")
target_include_directories(fgsqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
