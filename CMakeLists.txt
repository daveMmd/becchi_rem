cmake_minimum_required(VERSION 3.8)
#cmake_minimum_required(VERSION 2.8)

#set(CMAKE_C_COMPILER /usr/bin/g++)
#set(CMAKE_CXX_COMPILER /usr/local/bin/g++-9)
project(becchi_regex)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-w -g" )
set(CMAKE_CXX_FLAGS "-w -g -lboost_serialization")
#set(CMAKE_CXX_FLAGS "-fopenmp -w")
#set(CMAKE_CXX_FLAGS "-fopenmp -w -O4 -I.." )

#使用boost 库
#include_directories(/usr/local/Cellar/boost/1.70.0/include)
#link_directories(/usr/local/Cellar/boost/1.70.0/lib/)
find_package(Boost)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "FATAL: Boost library not found!")
endif()


set(stdinc stdinc.cpp)
set(int_set ${stdinc} int_set.cpp)
set(linked_set ${stdinc} linked_set.cpp)
set(wgraph ${stdinc} wgraph.cpp)
set(partition ${stdinc} partition.cpp)
set(dheap ${stdinc} dheap.cpp)
set(dfa ${stdinc} dfa.cpp)
set(nfa ${stdinc} ${dfa} nfa.cpp)
set(subset ${stdinc} ${dfa} ${nfa} subset.cpp)
set(hybrid_fa ${stdinc} ${nfa} ${dfa} hybrid_fa.cpp)
set(trace ${stdinc} trace.cpp)
set(parser ${stdinc} ${dfa} ${nfa} parser.cpp)

#test for component parse
add_subdirectory(component_tree)
#final for fpga
add_executable(bram_fpga2 dave_subset.cpp ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} main_fpga_2.cpp ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp hierarMerging.cpp dave_nfa.cpp hfadump.cpp set.cpp Fb_DFA.cpp Fb_DFA.h Fb_NFA.cpp Fb_NFA.h prefix_DFA.cpp)
target_link_libraries(bram_fpga2 parse)

add_executable(runtime_fpga runtime_fpga.cpp dave_subset.cpp ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp hierarMerging.cpp dave_nfa.cpp hfadump.cpp set.cpp Fb_DFA.cpp Fb_DFA.h Fb_NFA.cpp Fb_NFA.h prefix_DFA.cpp)

add_executable(test_dave test_dave.cpp dave_subset.cpp ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp hierarMerging.cpp dave_nfa.cpp hfadump.cpp set.cpp Fb_DFA.cpp Fb_DFA.h Fb_NFA.cpp Fb_NFA.h prefix_DFA.cpp)
target_link_libraries(test_dave parse)
#target_link_libraries(boost_serialization)