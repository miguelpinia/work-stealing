add_executable(app app.cpp)
target_link_libraries(app
  PRIVATE
  ws_library
  nlohmann_json::nlohmann_json
  )
