
set(DATA_DESTINATION "${CMAKE_BINARY_DIR}/src/data")

# Create directory if it doesn't exist
file(MAKE_DIRECTORY ${DATA_DESTINATION})

# Copy all files from data to destination
file(GLOB DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(DATA_FILE ${DATA_FILES})
    get_filename_component(FILENAME ${DATA_FILE} NAME)
    if(NOT FILENAME STREQUAL "CMakeLists.txt")
        configure_file(${DATA_FILE} ${DATA_DESTINATION}/${FILENAME} COPYONLY)
    endif()
endforeach()

message(STATUS "created commands for copying data")


