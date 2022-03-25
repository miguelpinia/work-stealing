file(GLOB srcs "${workStealing_SOURCE_DIR}/src/*.cpp")
file(GLOB hdrs "${workStealing_SOURCE_DIR}/include/ws/*.hpp")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_library(ws_library ${srcs} ${hdrs})

target_link_libraries(ws_library
  PRIVATE
  nlohmann_json::nlohmann_json
  Threads::Threads)

target_include_directories(ws_library
  PUBLIC
  "${workStealing_SOURCE_DIR}/include"
  ${json_SOURCE_DIR}
  )

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${HEADER_LIST}
  )
