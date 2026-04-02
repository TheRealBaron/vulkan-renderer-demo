
function(get_shader_stage_keyword STAGE_KEYWORD RESULT_VAR)
    if(${STAGE_KEYWORD} STREQUAL "VERTEX")
        set(${RESULT_VAR} "vert" PARENT_SCOPE)
    elseif(${STAGE_KEYWORD} STREQUAL "FRAGMENT")
        set(${RESULT_VAR} "frag" PARENT_SCOPE)
    elseif(${STAGE_KEYWORD} STREQUAL "COMPUTE")
        set(${RESULT_VAR} "comp" PARENT_SCOPE)
    elseif(${STAGE_KEYWORD} STREQUAL "GEOMETRY")
        set(${RESULT_VAR} "geom" PARENT_SCOPE)
    elseif(${STAGE_KEYWORD} STREQUAL "TESSC")
        set(${RESULT_VAR} "tesc" PARENT_SCOPE)
    elseif(${STAGE_KEYWORD} STREQUAL "TESSE")
        set(${RESULT_VAR} "tese" PARENT_SCOPE)
    elseif(${STAGE_KEYWORD} STREQUAL "RAYGEN")
        set(${RESULT_VAR} "rgen" PARENT_SCOPE)
    elseif(${STAGE_KEYWORD} STREQUAL "RAYCHIT")
        set(${RESULT_VAR} "rchit" PARENT_SCOPE)
    elseif(${STAGE_KEYWORD} STREQUAL "RAYMISS")
        set(${RESULT_VAR} "rmiss" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Unknown shader stage keyword: ${STAGE_KEYWORD}")
    endif()
endfunction()


function(compile_shaders TARGET_NAME)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs VERTEX FRAGMENT)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(ALL_OUTPUT_SPV "")
    foreach(SHADER_TYPE IN LISTS multiValueArgs)

        set(FILES ${ARGS_${SHADER_TYPE}})

        if(FILES)
            get_shader_stage_keyword(${SHADER_TYPE} SHADER_STAGE)

            foreach(SHADER_FILE ${FILES})
                get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)

                set(OUTPUT_SPV  "${CMAKE_BINARY_DIR}/src/shaders/${SHADER_NAME}.spv")
                set(SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_FILE}")

                add_custom_command(
                    OUTPUT ${OUTPUT_SPV}
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
                    COMMAND Vulkan::glslc 
                        -fshader-stage=${SHADER_STAGE}
                        "${SOURCE_FILE}"
                        -o "${OUTPUT_SPV}"
                    DEPENDS "${SOURCE_FILE}"
                    COMMENT "Compiling ${SHADER_STAGE} shader: ${SHADER_NAME}.spv"
                    VERBATIM
                )
                list(APPEND ALL_OUTPUT_SPV ${OUTPUT_SPV})
            endforeach()

        endif() 
    endforeach()

    message(STATUS "created commands for compiling shaders: ${ALL_OUTPUT_SPV}")

    add_custom_target(compile_all_shaders ALL DEPENDS ${ALL_OUTPUT_SPV})
    add_dependencies(${TARGET_NAME} compile_all_shaders)
endfunction()


add_executable(vkhello 
    main.cpp
    myapp.cpp
    core/GraphicsContext.cpp
    core/Swapchain.cpp
    core/Shader.cpp
    core/PipelineBuilder.cpp
    core/PipelineManager.cpp
    core/CommandManager.cpp
    core/FrameManager.cpp
    resources/ReadonlyBuffer.cpp
    resources/DepthBuffer.cpp
    resources/UniformBuffer.cpp
    utils/UtilObjects.cpp
)

target_include_directories(vkhello PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

compile_shaders(vkhello
    FRAGMENT fragment.glsl
    VERTEX vertex.glsl
)


target_link_libraries(vkhello 
    PRIVATE 
        Vulkan::Vulkan
        glfw
        glm::glm
        GPUOpen::VulkanMemoryAllocator
)
