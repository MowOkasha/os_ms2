# # cmake_minimum_required(VERSION 3.10)
# # project(ProcessScheduler C)

# # set(CMAKE_C_STANDARD 99)

# # # Find GTK package
# # find_package(PkgConfig REQUIRED)
# # pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# # # Include directories
# # include_directories(${GTK3_INCLUDE_DIRS})

# # # Link directories
# # link_directories(${GTK3_LIBRARY_DIRS})

# # # Add compiler flags
# # add_definitions(${GTK3_CFLAGS_OTHER})

# # # Add source files
# # set(SOURCES
# #     main.c
# #     gui.c
# # )

# # # Create executable
# # add_executable(ProcessScheduler ${SOURCES})

# # # <<< copy Program_*.txt into build/ <<<
# # file(GLOB PROG_FILES "${CMAKE_SOURCE_DIR}/Program_*.txt")
# # file(COPY ${PROG_FILES} DESTINATION ${CMAKE_BINARY_DIR})

# # # Link libraries
# # target_link_libraries(ProcessScheduler ${GTK3_LIBRARIES} pthread)
# cmake_minimum_required(VERSION 3.10)
# project(ProcessScheduler C)

# set(CMAKE_C_STANDARD 99)

# # Find GTK3
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# include_directories(${GTK3_INCLUDE_DIRS})
# link_directories(${GTK3_LIBRARY_DIRS})
# add_definitions(${GTK3_CFLAGS_OTHER})

# # Find pthread
# find_package(Threads REQUIRED)

# # Add executable
# add_executable(ProcessScheduler
#     gui.c
#     main.c
# )

# # Link libraries
# target_link_libraries(ProcessScheduler
#     ${GTK3_LIBRARIES}
#     Threads::Threads
# )





# cmake_minimum_required(VERSION 3.10)
# project(ProcessScheduler C)

# set(CMAKE_C_STANDARD 99)

# # Find GTK3
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# include_directories(${GTK3_INCLUDE_DIRS})
# link_directories(${GTK3_LIBRARY_DIRS})
# add_definitions(${GTK3_CFLAGS_OTHER})

# # Find pthread
# find_package(Threads REQUIRED)

# # Add executable with ALL source files
# add_executable(ProcessScheduler
#     gui.c
#     main.c

#     # Add any other .c files containing the missing functions
# )

# # Link libraries
# target_link_libraries(ProcessScheduler
#     ${GTK3_LIBRARIES}
#     Threads::Threads
# )

cmake_minimum_required(VERSION 3.10)
project(ProcessScheduler C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)      # for IDE support

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

find_package(Threads REQUIRED)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(   ${GTK3_LIBRARY_DIRS})
add_definitions(    ${GTK3_CFLAGS_OTHER})

add_executable(ProcessScheduler
    main.c
    gui.c
    memory.c    # implements allocate_memory, load_instructions, …
    queue.c     # implements init_queue, enqueue, dequeue
    instr.c     # implements execute_instruction
    log.c       # implements log_cycle
)

target_link_libraries(ProcessScheduler
    ${GTK3_LIBRARIES}
    Threads::Threads
)

# Copy all Program_*.txt into the build folder
file(GLOB PROG_FILES "${CMAKE_SOURCE_DIR}/Program_*.txt")
file(COPY ${PROG_FILES} DESTINATION ${CMAKE_BINARY_DIR})