project(MySharedPointer)

aux_source_directory(${PROJECT_SOURCE_DIR} SOURCE)

add_executable(SharedPtr ${SOURCE})

target_include_directories(SharedPtr PUBLIC ${PROJECT_SOURCE_DIR})