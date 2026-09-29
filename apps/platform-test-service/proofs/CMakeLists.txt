add_esbmc_proof(platform-test-service-build-info
    SOURCES proof_build_info.cpp ../src/build_info.cpp
    INCLUDE_DIRECTORIES ../include
    UNWIND 32
    TIMEOUT 60
)
