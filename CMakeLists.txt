cmake_minimum_required(VERSION 3.16)

project(QDiskInfo VERSION 0.3 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wuseless-cast -Wconversion -Wsign-conversion -Wshadow")

if(NOT DEFINED QT_VERSION_MAJOR)
    set(QT_VERSION_MAJOR 6)
endif()

if(ENABLE_TRANSLATIONS)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
endif()

file(GLOB_RECURSE PROJECT_SOURCES
    src/*.h
    src/*.cpp
    src/*.ui
    src/resources.qrc
)

if(INCLUDE_OPTIONAL_RESOURCES)
    set(PROJECT_SOURCES ${PROJECT_SOURCES} dist/theme/theme_resources.qrc)
endif()

if(ENABLE_TRANSLATIONS)
    file(GLOB TS_FILES "${CMAKE_SOURCE_DIR}/translations/*.ts")
    if(QT_VERSION_MAJOR GREATER_EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 7)
        qt_add_translations(TARGETS ${PROJECT_NAME}
                TS_FILES ${TS_FILES}
                SOURCES ${PROJECT_SOURCES}
                QM_FILES_OUTPUT_VARIABLE QM_FILES
                LUPDATE_OPTIONS -no-obsolete -locations none
        )
    else()
        qt_add_translation(QM_FILES ${TS_FILES})
    endif()
    set(TRANSLATIONS_QRC "${CMAKE_BINARY_DIR}/translations.qrc")
    file(WRITE ${TRANSLATIONS_QRC} "<RCC>\n  <qresource prefix=\"/translations\">\n")
    foreach(QM_FILE ${QM_FILES})
        get_filename_component(QM_FILE_NAME ${QM_FILE} NAME)
        file(APPEND ${TRANSLATIONS_QRC} "    <file>${QM_FILE_NAME}</file>\n")
    endforeach()
    file(APPEND ${TRANSLATIONS_QRC} "  </qresource>\n</RCC>\n")
    list(APPEND PROJECT_SOURCES ${TRANSLATIONS_QRC})
endif()

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_VERSION_MAJOR=${${PROJECT_NAME}_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${${PROJECT_NAME}_VERSION_MINOR}
    ENABLE_TRANSLATIONS=$<BOOL:${ENABLE_TRANSLATIONS}>
    INCLUDE_OPTIONAL_RESOURCES=$<BOOL:${INCLUDE_OPTIONAL_RESOURCES}>
    CHARACTER_IS_RIGHT=$<BOOL:${CHARACTER_IS_RIGHT}>
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES "dist/${PROJECT_NAME}.desktop"
        DESTINATION "share/applications")
install(FILES "dist/${PROJECT_NAME}.svg"
        DESTINATION "share/icons/hicolor/scalable/apps")

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
include(FeatureSummary)
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
