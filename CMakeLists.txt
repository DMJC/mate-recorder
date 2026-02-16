cmake_minimum_required(VERSION 3.10)
project(AudioRecorder)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(ALSA REQUIRED alsa)

include_directories(${GTKMM_INCLUDE_DIRS} ${ALSA_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS} ${ALSA_LIBRARY_DIRS})

add_executable(audiorecorder main.cpp)

target_link_libraries(audiorecorder 
    ${GTKMM_LIBRARIES}
    ${ALSA_LIBRARIES}
    portaudio
    sndfile
)
