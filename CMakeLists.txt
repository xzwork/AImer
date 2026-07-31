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
find_package(OpenVINO REQUIRED)

# onnxruntime
set(ORT_DIR ${PROJECT_SOURCE_DIR}/deps/onnxruntime)
set(ORT_DLL_FILES
        ${ORT_DIR}/lib/onnxruntime.dll
        ${ORT_DIR}/lib/onnxruntime_providers_shared.dll
        ${ORT_DIR}/lib/onnxruntime_providers_cuda.dll
)

# OpenVINO runtime
set(OpenVINO_DIR ${PROJECT_SOURCE_DIR}/deps/openvino)
# tbb dlls
file(GLOB OpenVINO_TBB_DLL_FILES
        ${OpenVINO_DIR}/tbb*.dll
        ${OpenVINO_DIR}/tbbmalloc*.dll
)

# IbInputSimulator
set(IB_DIR ${PROJECT_SOURCE_DIR}/deps/IbInputSimulator)

# CUDNN
set(CUDNN_LIB_DIR CACHE PATH "Path to cuDNN library directory")

# EUI-NEO - Disable examples and building extra tests
set(EUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(EUI_BUILD_PLAYGROUND OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/EUI-NEO EXCLUDE_FROM_ALL)

# AImer
add_executable(AImer
        src/main.cpp
        src/Games.cpp
        src/autoAim.cpp
        src/ScreenCapture.cpp
        src/MouseController.cpp

        src/launcher/Launcher.cpp
        src/launcher/interface.cpp

        src/detector/Detector.cpp
        src/detector/OnnxDetector.cpp
        src/detector/OpenvinoDetector.cpp
)

target_include_directories(AImer PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${OpenVINO_INCLUDE_DIRS}
        ${IB_DIR}
        ${ORT_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)

target_link_directories(AImer PRIVATE
        ${CUDNN_LIB_DIR}
        ${ORT_DIR}/lib
)

target_link_libraries(AImer PRIVATE
        CLI11::CLI11
        eui_neo
        yaml-cpp
        d3d11
        dxgi
        ${OpenCV_LIBS}
        cudnn64_9.lib
        openvino::runtime
        onnxruntime.lib
        onnxruntime_providers_cuda.lib
        onnxruntime_providers_shared.lib
)

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
add_custom_command(TARGET AImer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${OpenVINO_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>"
        "$<TARGET_FILE_DIR:AImer>"
        COMMENT "Copying OpenVINO runtime dlls ($<CONFIG>)"
)

# copy games.yaml
add_custom_command(TARGET AImer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/games.yaml"
        "$<TARGET_FILE_DIR:AImer>"
        COMMENT "Copying games.yaml to output directory"
)

# copy TBB dlls
foreach (DLL_FILE ${OpenVINO_TBB_DLL_FILES})
    add_custom_command(TARGET AImer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL_FILE}
            $<TARGET_FILE_DIR:AImer>
            COMMENT "Copying TBB dlls to output directory"
    )
endforeach ()