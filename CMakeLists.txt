cmake_minimum_required(VERSION 3.15)
project(AImer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# CLI11
add_subdirectory(deps/CLI11 EXCLUDE_FROM_ALL)

# yaml-cpp
add_subdirectory(deps/yaml-cpp EXCLUDE_FROM_ALL)

# OpenCV
find_package(OpenCV REQUIRED)
option(AIMER_WITH_OPENVINO "Enable the optional OpenVINO detector" OFF)
if(AIMER_WITH_OPENVINO)
    find_package(OpenVINO REQUIRED)
endif()

# onnxruntime
set(ORT_DIR ${PROJECT_SOURCE_DIR}/deps/onnxruntime)
set(ORT_DLL_FILES
        ${ORT_DIR}/lib/onnxruntime.dll
        ${ORT_DIR}/lib/onnxruntime_providers_shared.dll
        ${ORT_DIR}/lib/onnxruntime_providers_cuda.dll
)

# Keep the optional bundled runtime location separate from OpenVINO_DIR,
# which belongs to find_package(OpenVINO).
if(AIMER_WITH_OPENVINO)
    set(AIMER_OPENVINO_RUNTIME_DIR "${PROJECT_SOURCE_DIR}/deps/openvino"
            CACHE PATH "OpenVINO runtime root containing bin/Release and bin/Debug")
    file(GLOB OpenVINO_TBB_DLL_FILES
            "${AIMER_OPENVINO_RUNTIME_DIR}/tbb*.dll"
            "${AIMER_OPENVINO_RUNTIME_DIR}/tbbmalloc*.dll"
    )
endif()

# IbInputSimulator
set(IB_DIR ${PROJECT_SOURCE_DIR}/deps/IbInputSimulator)

# Independent desktop input diagnostic: no model, capture or GPU dependencies.
add_executable(MouseInputCheck EXCLUDE_FROM_ALL src/controller/MouseInputCheck.cpp)
target_include_directories(MouseInputCheck PRIVATE ${IB_DIR})
target_link_libraries(MouseInputCheck PRIVATE user32)
add_custom_command(TARGET MouseInputCheck POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${IB_DIR}/IbInputSimulator.dll" "$<TARGET_FILE_DIR:MouseInputCheck>"
        VERBATIM
)

# cuDNN is loaded by ONNX Runtime's CUDA provider, not linked by AImer.
# Leave empty when CUDA/cuDNN runtime directories are already on PATH.
set(CUDNN_BIN_DIR "" CACHE PATH "Directory containing CUDA-compatible cuDNN 9 DLLs")
if(CUDNN_BIN_DIR)
    if(NOT EXISTS "${CUDNN_BIN_DIR}/cudnn64_9.dll")
        message(FATAL_ERROR "CUDNN_BIN_DIR must contain cudnn64_9.dll")
    endif()
    file(GLOB CUDNN_DLL_FILES CONFIGURE_DEPENDS "${CUDNN_BIN_DIR}/cudnn*64_9.dll")
endif()

# EUI-NEO - Disable examples and building extra tests
set(EUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(EUI_BUILD_PLAYGROUND OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/EUI-NEO EXCLUDE_FROM_ALL)

# AImer
add_executable(AImer
        src/capture/ScreenCapture.cpp
        src/controller/MouseController.cpp
        src/controller/VirtualHidBackend.cpp

        src/AImer-DL/launcher/Launcher.cpp
        src/AImer-DL/launcher/interface.cpp

        src/AImer-DL/main.cpp
        src/AImer-DL/Games.cpp
        src/AImer-DL/autoAim.cpp
        src/AImer-DL/detector/Detector.cpp
        src/AImer-DL/detector/OnnxDetector.cpp
)

target_include_directories(AImer PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${IB_DIR}
        ${ORT_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
)

target_link_directories(AImer PRIVATE
        ${ORT_DIR}/lib
)

target_link_libraries(AImer PRIVATE
        CLI11::CLI11
        eui_neo
        yaml-cpp
        d3d11
        dxgi
        user32
        ${OpenCV_LIBS}
        onnxruntime.lib
)

if(AIMER_WITH_OPENVINO)
    target_sources(AImer PRIVATE src/AImer-DL/detector/OpenvinoDetector.cpp)
    target_compile_definitions(AImer PRIVATE AIMER_WITH_OPENVINO=1)
    target_link_libraries(AImer PRIVATE openvino::runtime)
endif()

# Copy all cuDNN components when explicitly configured; CUDA runtime stays on PATH.
if(CUDNN_BIN_DIR)
    add_custom_command(TARGET AImer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CUDNN_DLL_FILES}
            $<TARGET_FILE_DIR:AImer>
            COMMENT "Copying cuDNN 9 runtime DLLs to output directory"
            VERBATIM
    )
endif()

# copy onnxruntime dlls
foreach (DLL_FILE ${ORT_DLL_FILES})
    add_custom_command(TARGET AImer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL_FILE}
            $<TARGET_FILE_DIR:AImer>
            COMMENT "Copying ONNX dlls to output directory"
    )
endforeach ()

# copy IbInputSimulator dll
add_custom_command(TARGET AImer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${IB_DIR}/IbInputSimulator.dll
        $<TARGET_FILE_DIR:AImer>
        COMMENT "Copying IbInputSimulator dll to output directory"
)

# copy OpenVINO runtime dlls (debug/release based on build config)
if(AIMER_WITH_OPENVINO)
add_custom_command(TARGET AImer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${AIMER_OPENVINO_RUNTIME_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>"
        "$<TARGET_FILE_DIR:AImer>"
        COMMENT "Copying OpenVINO runtime dlls ($<CONFIG>)"
)

endif()

# copy games.yaml
add_custom_command(TARGET AImer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/games.yaml"
        "$<TARGET_FILE_DIR:AImer>"
        COMMENT "Copying games.yaml to output directory"
)

# copy TBB dlls
if(AIMER_WITH_OPENVINO)
foreach (DLL_FILE ${OpenVINO_TBB_DLL_FILES})
    add_custom_command(TARGET AImer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL_FILE}
            $<TARGET_FILE_DIR:AImer>
            COMMENT "Copying TBB dlls to output directory"
    )
endforeach ()

endif()

# AImer-aimlab (traditional CV-based aimlab assistant)
add_executable(AImer-aimlab
        src/AImer-aimlab/main.cpp
        src/AImer-aimlab/autoAim.cpp
        src/capture/ScreenCapture.cpp
        src/controller/MouseController.cpp
        src/controller/VirtualHidBackend.cpp
)

target_include_directories(AImer-aimlab PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${IB_DIR}
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/aimlab
)

target_link_libraries(AImer-aimlab PRIVATE
        CLI11::CLI11
        yaml-cpp
        ${OpenCV_LIBS}
        d3d11
        dxgi
        user32
)

# copy IbInputSimulator dll
add_custom_command(TARGET AImer-aimlab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${IB_DIR}/IbInputSimulator.dll
        $<TARGET_FILE_DIR:AImer-aimlab>
        COMMENT "Copying IbInputSimulator dll to output directory"
)

foreach(mouse_target AImer AImer-aimlab)
    add_custom_command(TARGET ${mouse_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/mouse.yaml" "$<TARGET_FILE_DIR:${mouse_target}>"
            VERBATIM)
endforeach()

# Driver launcher uses the user's existing Start-AImer.cmd entry point.
add_executable(VirtualHidCheck EXCLUDE_FROM_ALL
        tests/virtual_hid/VirtualHidCheck.cpp
        src/controller/VirtualHidBackend.cpp
        src/controller/MouseController.cpp)
target_include_directories(VirtualHidCheck PRIVATE ${IB_DIR})
target_link_libraries(VirtualHidCheck PRIVATE yaml-cpp user32)
add_dependencies(AImer VirtualHidCheck)
add_custom_command(TARGET AImer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/scripts/virtual-hid" "$<TARGET_FILE_DIR:AImer>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/drivers/VirtualHidMouse/install.ps1"
        "$<TARGET_FILE_DIR:AImer>/Install-VirtualHidDriver.ps1"
        VERBATIM)
