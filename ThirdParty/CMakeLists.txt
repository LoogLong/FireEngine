


add_subdirectory(Eigen)

if(NOT TARGET stb)
    include(stb.cmake)
    set_target_properties(stb PROPERTIES FOLDER "ThirdParty")
endif()