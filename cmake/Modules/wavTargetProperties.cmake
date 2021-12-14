set(wav_compile_features c_std_99)

set(wav_compile_definitions
  __STDC_FORMAT_MACROS
  __STDC_LIMIT_MACROS
  __STDC_CONSTANT_MACROS
  )

if(NOT DEFINED wav_c_flags)
  set(wav_c_flags "")
  include(CheckCCompilerFlag)
  if(${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
  elseif(${CMAKE_C_COMPILER_ID} MATCHES "^(GNU|.*Clang)$")
    foreach(flag -fno-strict-aliasing
                -Wall
                -Wcast-align
                -Wduplicated-branches
                -Wduplicated-cond
                -Wextra
                -Wformat=2
                -Wmissing-include-dirs
                -Wnarrowing
                -Wpointer-arith
                -Wshadow
                -Wuninitialized
                -Wwrite-strings
                -Wno-multichar
                -Wno-format-nonliteral
                -Wno-format-truncation
                -Werror=discarded-qualifiers
                -Werror=ignored-qualifiers
                -Werror=implicit
                -Werror=implicit-function-declaration
                -Werror=implicit-int
                -Werror=init-self
                -Werror=incompatible-pointer-types
                -Werror=int-conversion
                -Werror=return-type
                -Werror=strict-prototypes
                )
      check_c_compiler_flag(${flag} wav_has_c_flag_${flag})
      if(wav_has_c_flag_${flag})
        list(APPEND wav_c_flags ${flag})
      endif()
    endforeach()
  endif()
  set(wav_c_flags ${wav_c_flags} CACHE INTERNAL "C compiler flags")
endif()

if(${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
elseif(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    set(wav_compile_options_release -fomit-frame-pointer -march=native -mtune=native -fvisibility=hidden)
elseif(${CMAKE_C_COMPILER_ID} MATCHES "^.*Clang$")
    set(wav_compile_options_release -fomit-frame-pointer -fvisibility=hidden)
endif()
