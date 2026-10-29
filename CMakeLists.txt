cmake_minimum_required(VERSION 3.21)

project(SatelliteBandPreview VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

add_executable(SatelliteBandPreview
    src/main.cpp
    src/image_processor.h
    src/image_processor.cpp
    src/image_canvas.h
    src/image_canvas.cpp
    src/histogram_widget.h
    src/histogram_widget.cpp
    src/demo_image.h
    src/demo_image.cpp
    src/preview_window.h
    src/preview_window.cpp
)

target_link_libraries(SatelliteBandPreview PRIVATE Qt6::Widgets)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        set(WINDEPLOYQT_ENVIRONMENT)
        if(MSVC)
            if(DEFINED ENV{VSINSTALLDIR})
                list(APPEND WINDEPLOYQT_ENVIRONMENT
                    "VSINSTALLDIR=$ENV{VSINSTALLDIR}"
                    "VCINSTALLDIR=$ENV{VCINSTALLDIR}"
                )
            else()
                find_program(VSWHERE_EXECUTABLE
                    NAMES vswhere
                    HINTS "$ENV{ProgramFiles}/../Program Files (x86)/Microsoft Visual Studio/Installer"
                )
                if(VSWHERE_EXECUTABLE)
                    execute_process(
                        COMMAND "${VSWHERE_EXECUTABLE}"
                            -latest
                            -products "*"
                            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
                            -property installationPath
                        OUTPUT_VARIABLE VISUAL_STUDIO_INSTALLATION
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                    )
                    if(VISUAL_STUDIO_INSTALLATION)
                        list(APPEND WINDEPLOYQT_ENVIRONMENT
                            "VSINSTALLDIR=${VISUAL_STUDIO_INSTALLATION}"
                            "VCINSTALLDIR=${VISUAL_STUDIO_INSTALLATION}/VC"
                        )
                    endif()
                endif()
            endif()
        endif()

        add_custom_command(TARGET SatelliteBandPreview POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E env
                ${WINDEPLOYQT_ENVIRONMENT}
                "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --compiler-runtime
                "$<TARGET_FILE:SatelliteBandPreview>"
            COMMENT "Deploying Qt runtime next to SatelliteBandPreview"
            VERBATIM
        )
    endif()
endif()

if(MSVC)
    target_compile_options(SatelliteBandPreview PRIVATE /W4 /permissive-)
else()
    target_compile_options(SatelliteBandPreview PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME image_processor_self_test
        COMMAND SatelliteBandPreview --self-test
    )
endif()
