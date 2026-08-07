# cmake/CompilerFlags.cmake

function(apply_compiler_flags TARGET_NAME)
    # Apply strict warning flags according to compiler vendor
    if(MSVC)
        target_compile_options(${TARGET_NAME} INTERFACE
            /W4
            /WX         # Treat warnings as errors
            /permissive- # Enforce strict standard conformance mode
        )
    else() # GCC / Clang
        target_compile_options(${TARGET_NAME} INTERFACE
            -Wall
            -Wextra
            -Wpedantic
            -Werror             # Treat warnings as errors
            -Wshadow            # Warn when a local variable shadows another
            -Wnon-virtual-dtor  # Warn about non-virtual destructors in base classes
            -Wold-style-cast    # Force C++ style casts (static_cast, etc.)
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
        )
    endif()
endfunction()

function(enable_sanitizers TARGET_NAME)
    if(NOT MSVC)
        # Enable AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)
        target_compile_options(${TARGET_NAME} INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${TARGET_NAME} INTERFACE
            -fsanitize=address,undefined
        )
    endif()
endfunction()