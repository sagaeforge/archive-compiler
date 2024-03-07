cmake_minimum_required ( VERSION 3.25 )
project (
  "n-cc_application"
  VERSION 0.1.0
  DESCRIPTION "n-cc application"
  LANGUAGES CXX
)

add_executable ( ${PROJECT_NAME}
    "application.cpp"
)

target_link_libraries( ${PROJECT_NAME}
  PRIVATE
    "nugdev_ini-parser"
)