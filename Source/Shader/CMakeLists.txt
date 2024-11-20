

if(NOT TARGET Shader)
    include(Shader.cmake)
    set_target_properties(Shader PROPERTIES FOLDER "Shader")
endif()