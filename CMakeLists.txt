cmake_minimum_required(VERSION 3.10)
project(AdServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost components
find_package(Boost REQUIRED COMPONENTS system thread)
# Note: Boost.Beast is header-only, so no additional linking needed

# Create executable
add_executable(adserver adserver.cpp)

# Link libraries
target_link_libraries(adserver 
    Boost::system 
    Boost::thread 
    pthread
)

# Set compiler flags for optimization
target_compile_options(adserver PRIVATE 
    -O2 
    -Wall 
    -Wextra 
    -DNDEBUG
)

# Install target
install(TARGETS adserver DESTINATION bin) 