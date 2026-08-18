add_library(kern_backend
    MachIR.cpp
    MachIRDump.cpp
    InstructionSelector.cpp
    RegisterAllocator.cpp
    Emitter.cpp
    PeepholeOptimizer.cpp
    X86Backend.cpp
    TargetBackend.cpp
)
target_include_directories(kern_backend PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(kern_backend PUBLIC kern_lir kern_support)
