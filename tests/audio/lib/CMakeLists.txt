# Glob all source and header files in this folder
file(GLOB TRANSFER_LIB_HEADERS "*.h")
file(GLOB TRANSFER_LIB_SOURCES "*.cpp")

# Add them to a parent-visible variable
# PARENT_SCOPE ensures the variable is available in the parent CMake
set(TRANSFER_LIB_HEADERS ${TRANSFER_LIB_HEADERS} PARENT_SCOPE)
set(TRANSFER_LIB_SOURCES ${TRANSFER_LIB_SOURCES} PARENT_SCOPE)
