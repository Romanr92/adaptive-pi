add_esbmc_proof(ara-core-error-code
    SOURCES proof_error_code.cpp
    INCLUDE_DIRECTORIES ../include
    UNWIND 32
    TIMEOUT 60
)
