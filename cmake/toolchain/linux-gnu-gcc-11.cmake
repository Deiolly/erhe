set(CMAKE_C_COMPILER "gcc-11")
set(CMAKE_CXX_COMPILER "g++-11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=class-memaccess")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=comment")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-copy")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=implicit-fallthrough")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=overloaded-virtual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=redundant-move")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=reorder")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=type-limits")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-contract=off")
