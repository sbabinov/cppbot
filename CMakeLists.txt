cmake_minimum_required(VERSION 3.14)
project(cppbot
    VERSION 1.0.0
    DESCRIPTION "Library for creating Telegram bots"
    LANGUAGES CXX
)

#---------------------------------------------------------------------------------------------
# Language settings
#---------------------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#---------------------------------------------------------------------------------------------
# General options
#---------------------------------------------------------------------------------------------

include(cmake/utils.cmake)
include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)

# CPPBOT_SHARED_LIBS (library will build as shared if defined)
option(CPPBOT_BUILD_EXAMPLES "Build cppbot examples" ON)
option(CPPBOT_BUILD_DOCS "Build cppbot documentation" OFF)
option(CPPBOT_INSTALL "Generate targer for installing cppbot" ${is_top_level})
set_if_undefined(CPPBOT_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/cppbot-${PROJECT_VERSION}" CACHE STRING
    "Install path for cppbot package-related CMake files"
)

if(DEFINED CPPBOT_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ${CPPBOT_SHARED_LIBS})
endif()

add_library(cppbot)
add_library(cppbot:cppbot ALIAS cppbot)

#---------------------------------------------------------------------------------------------
# Cppbot dependencies
#---------------------------------------------------------------------------------------------

find_package(Boost 1.86.0 REQUIRED COMPONENTS system)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(cppbot
    PUBLIC
        Boost::boost
        Boost::system
        nlohmann_json::nlohmann_json
        OpenSSL::SSL
        OpenSSL::Crypto
)

#---------------------------------------------------------------------------------------------
# Cppbot sources
#---------------------------------------------------------------------------------------------

set_if_undefined(CMAKE_CXX_VISIBILITY_PRESET hidden)
set_if_undefined(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include(GenerateExportHeader)
set(export_file_name "export_shared.h")

if(NOT BUILD_SHARED_LIBS)
    set(export_file_name "export_static.h")
endif()

generate_export_header(cppbot EXPORT_FILE_NAME include/cppbot/${export_file_name})

set(sources
    include/cppbot/export.h
    include/cppbot/cppbot.hpp
    include/cppbot/types.hpp
    include/cppbot/handlers.hpp
    include/cppbot/states.hpp
    src/cppbot.cpp
    src/types.cpp
    src/handlers.cpp
    src/states.cpp
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

#---------------------------------------------------------------------------------------------
# Cppbot target
#---------------------------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

target_sources(cppbot PRIVATE ${sources})
target_compile_definitions(cppbot
    PUBLIC
        "$<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:CPPBOT_STATIC_DEFINE>"
)

target_include_directories(cppbot
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set_target_properties(cppbot PROPERTIES
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
)

if(CPPBOT_INSTALL AND NOT CMAKE_SKIP_INSTALL_RULES)
    configure_package_config_file(cmake/cppbot-config.cmake.in cppbot-config.cmake
        INSTALL_DESTINATION "${CPPBOT_INSTALL_CMAKEDIR}")

    write_basic_package_version_file(cppbot-config-version.cmake
        COMPATIBILITY SameMajorVersion)

    install(TARGETS cppbot EXPORT cppbot_export
        RUNTIME COMPONENT cppbot
        LIBRARY COMPONENT cppbot NAMELINK_COMPONENT cppbot-dev
        ARCHIVE COMPONENT cppbot-dev
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )
    install(DIRECTORY include/
        TYPE INCLUDE
        COMPONENT cppbot-dev
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/cppbot/${export_file_name}"
        COMPONENT cppbot-dev
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mylib"
    )

    set(targets_file "cppbot-shared-targets.cmake")

    if(NOT BUILD_SHARED_LIBS)
        set(targets_file "cppbot-static-targets.cmake")
    endif()

    install(EXPORT cppbot_export
        COMPONENT cppbot-dev
        FILE "${targets_file}"
        DESTINATION "${CPPBOT_INSTALL_CMAKEDIR}"
        NAMESPACE cppbot::
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/cppbot-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/cpppbot-config-version.cmake"
        COMPONENT cppbot-dev
        DESTINATION "${CPPBOT_INSTALL_CMAKEDIR}"
    )

    if(MSVC)
        set(pdb_file "")
        set(pdb_file_destination "")

        if(BUILD_SHARED_LIBS)
            set(pdb_file "$<TARGET_PDB_FILE:mylib>")
            set(pdb_file_destination "${CMAKE_INSTALL_BINDIR}")
        else()
            # TARGET_PDB_FILE does not work for pdb file generated for static library build, determining it manually
            set(pdb_file "$<TARGET_FILE_DIR:cppbot>/$<TARGET_FILE_PREFIX:cppbot>$<TARGET_FILE_BASE_NAME:cppbot>.pdb")
            set(pdb_file_destination "${CMAKE_INSTALL_LIBDIR}")
        endif()

        install(FILES "${pdb_file}"
            COMPONENT cppbot-dev
            CONFIGURATIONS Debug RelWithDebInfo
            DESTINATION "${pdb_file_destination}"
            OPTIONAL
        )
    endif()
endif()

#----------------------------------------------------------------------------------------------------------------------
# other targets
#----------------------------------------------------------------------------------------------------------------------

if(CPPBOT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CPPBOT_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    doxygen_add_docs(docs include)
endif()
