cmake_minimum_required(VERSION 3.15)
project(AImer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# CLI11
include(FetchContent)
FetchContent_Declare(CLI11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git GIT_TAG v2.4.2)
FetchContent_MakeAvailable(CLI11)

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

# AImer
add_executable(AImer
        src/main.cpp
        src/autoAim.cpp
        src/detector/Detector.cpp
        src/detector/OnnxDetector.cpp
        src/detector/OpenvinoDetector.cpp
        src/GameSettings.cpp
        src/ScreenCapture.cpp
        src/MouseController.cpp
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
        ${OpenCV_LIBS}
        openvino::runtime
        CLI11::CLI11
        onnxruntime.lib
        onnxruntime_providers_shared.lib
        onnxruntime_providers_cuda.lib
        cudnn64_9.lib
        d3d11
        dxgi
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

# copy TBB dlls
foreach (DLL_FILE ${OpenVINO_TBB_DLL_FILES})
    add_custom_command(TARGET AImer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL_FILE}
            $<TARGET_FILE_DIR:AImer>
            COMMENT "Copying TBB dlls to output directory"
    )
endforeach ()