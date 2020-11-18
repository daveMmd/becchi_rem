cmake_minimum_required(VERSION 3.8)
#cmake_minimum_required(VERSION 2.8)

#set(CMAKE_C_COMPILER /usr/bin/g++)
#set(CMAKE_CXX_COMPILER /usr/local/bin/g++-9)
project(becchi_regex)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-w -g" )
set(CMAKE_CXX_FLAGS "-w -g")
#set(CMAKE_CXX_FLAGS "-fopenmp -w")
#set(CMAKE_CXX_FLAGS "-fopenmp -w -O4 -I.." )

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

add_executable(regex ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} main.cpp ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp)
add_executable(regex_dfas stdinc.cpp int_set.cpp linked_set.cpp dheap.cpp partition.cpp wgraph.cpp cache.cpp nfa.cpp dfa.cpp c_type.cpp ecdfa.cpp hybrid_fa.cpp subset.cpp parser.cpp dfas_memory.cpp fa_memory.cpp main_dfas.cpp trace.cpp)
add_executable(regex_nfa stdinc.cpp int_set.cpp linked_set.cpp dheap.cpp partition.cpp wgraph.cpp cache.cpp nfa.cpp dfa.cpp c_type.cpp ecdfa.cpp hybrid_fa.cpp subset.cpp parser.cpp dfas_memory.cpp fa_memory.cpp main_nfa.cpp trace.cpp)

add_executable(regex_tracegen Fb_NFA.cpp Fb_DFA.cpp dave_subset.cpp ${stdinc} ${int_set} ${linked_set} ${dheap} ${partition} ${wgraph} cache.cpp ${nfa} dfa.cpp c_type.cpp ecdfa.cpp hybrid_fa.cpp subset.cpp parser.cpp dfas_memory.cpp fa_memory.cpp main_tracegen.cpp trace.cpp hierarMerging.cpp)
add_executable(regex_gen ${stdinc} main_regexgen.cpp)
add_executable(regex_new stdinc.cpp int_set.cpp linked_set.cpp dheap.cpp subset.cpp partition.cpp wgraph.cpp nfa.cpp dfa.cpp parser.cpp rcdfa.cpp mdfa.cpp hybrid_fa.cpp main_new.cpp)

add_executable(regex_test2 dave_subset.cpp Fb_NFA.cpp Fb_DFA.cpp stdinc.cpp linked_set.cpp partition.cpp wgraph.cpp dheap.cpp subset.cpp nfa.cpp dfa.cpp int_set.cpp parser.cpp c_type.cpp ecdfa.cpp ecdfab.cpp rcdfa.cpp bitdfa.cpp rledfa.cpp orledfa.cpp main_test2.cpp hierarMerging.cpp)
add_executable(regex_nmdfa stdinc.cpp int_set.cpp linked_set.cpp subset.cpp dheap.cpp partition.cpp wgraph.cpp nfa.cpp dfa.cpp c_type.cpp ecdfa.cpp parser.cpp nmdfa.cpp main_nmdfa.cpp)
add_executable(regex_becchi stdinc.cpp int_set.cpp linked_set.cpp subset.cpp dheap.cpp partition.cpp wgraph.cpp nfa.cpp dfa.cpp c_type.cpp ecdfa.cpp parser.cpp becchi.cpp main_becchi.cpp)

#add_executable(regex_dave ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} main_dave.cpp ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp dave_util.cpp dave_nfa.cpp)# dave_nfa.cpp
#add_executable(test_dave test_dave.cpp)

add_executable(regex_hfa ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} main_testhfa.cpp ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp hierarMerging.cpp dave_nfa.cpp hfadump.cpp set.cpp)

#test for fpga (multi-DFA)
#add_executable(test_fpga ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} main_test_fpga.cpp ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp)
add_executable(test_fpga ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} main_test_fpga.cpp ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp hierarMerging.cpp dave_nfa.cpp hfadump.cpp set.cpp Fb_DFA.cpp Fb_DFA.h Fb_NFA.cpp Fb_NFA.h)
add_executable(bram_fpga ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} main_fpga.cpp ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp hierarMerging.cpp dave_nfa.cpp hfadump.cpp set.cpp Fb_DFA.cpp Fb_DFA.h Fb_NFA.cpp Fb_NFA.h)

#test for component parse
add_subdirectory(component_tree)
#final for fpga
add_executable(bram_fpga2 dave_subset.cpp ${stdinc} ${int_set} ${linked_set} ${partition} ${wgraph} ${dheap} ${dfa} ${nfa} c_type.cpp ecdfa.cpp ${subset} ${parser} main_fpga_2.cpp ${trace} ${hybrid_fa} cache.cpp fa_memory.cpp dfas_memory.cpp hierarMerging.cpp dave_nfa.cpp hfadump.cpp set.cpp Fb_DFA.cpp Fb_DFA.h Fb_NFA.cpp Fb_NFA.h)
target_link_libraries(bram_fpga2 parse)