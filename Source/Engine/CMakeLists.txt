
set(TARGET_NAME Engine)

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.hpp)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp)

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${TARGET_NAME} INTERFACE)
add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Public)



target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_THIRD_PARTY_DIR}/Eigen)
target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_THIRD_PARTY_DIR}/fbxsdk/include)

# 三方库link
target_link_libraries(${TARGET_NAME} PRIVATE stb)
target_link_libraries(${TARGET_NAME} PRIVATE Shader)

#linker
target_link_libraries(${TARGET_NAME} PRIVATE dxguid.lib)
target_link_libraries(${TARGET_NAME} PRIVATE dxgi.lib)
target_link_libraries(${TARGET_NAME} PRIVATE d3d12.lib)
target_link_libraries(${TARGET_NAME} PRIVATE d3dcompiler.lib)
target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_THIRD_PARTY_DIR}/fbxsdk/lib/vs2017/x64/debug/libfbxsdk-md.lib)
target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_THIRD_PARTY_DIR}/fbxsdk/lib/vs2017/x64/debug/libxml2-md.lib)
target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_THIRD_PARTY_DIR}/fbxsdk/lib/vs2017/x64/debug/zlib-md.lib)

# 按需拷贝！
# file(COPY ${PROJECT_THIRD_PARTY_DIR}/fbxsdk/lib/vs2017/x64/release/libfbxsdk.dll DESTINATION ${OUTPUT_DIR_DEBUG}/)