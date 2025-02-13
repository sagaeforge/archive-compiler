# VCPKG 툴체인 파일 설정
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake
    CACHE STRING "Vcpkg toolchain file")

# 플랫폼별 설정
if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
elseif(APPLE)
    set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "")
elseif(UNIX)
    set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "")
endif()

find_package(ICU REQUIRED COMPONENTS uc i18n data)
find_package(RapidJSON CONFIG REQUIRED)