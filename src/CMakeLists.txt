file(GLOB srcs "${workStealing_SOURCE_DIR}/src/*.cpp")
file(GLOB hdrs "${workStealing_SOURCE_DIR}/include/ws/*.hpp")

add_library(ws_library ${srcs} ${hdrs})
target_include_directories(ws_library PUBLIC "${workStealing_SOURCE_DIR}/include")

source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header Files"
        FILES ${HEADER_LIST}
)
