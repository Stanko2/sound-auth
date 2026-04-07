# Glob all source and header files in this folder
file(GLOB_RECURSE TRANSFER_LIB_HEADERS "*.h")
# Glob all cpp sources recursively so subdirectories (e.g. ModulationStrategies) are included
file(GLOB_RECURSE TRANSFER_LIB_SOURCES "*.cpp")

# Add them to a parent-visible variable
# PARENT_SCOPE ensures the variable is available in the parent CMake
set(TRANSFER_LIB_HEADERS ${TRANSFER_LIB_HEADERS} PARENT_SCOPE)
set(TRANSFER_LIB_SOURCES ${TRANSFER_LIB_SOURCES} PARENT_SCOPE)
