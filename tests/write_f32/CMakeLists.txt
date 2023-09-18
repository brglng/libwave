add_executable(write-f32 main.c)
target_link_libraries(write-f32
    wave::wave
    $<$<PLATFORM_ID:Linux>:m>
    )
target_include_directories(write-f32 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(write-f32 PRIVATE ${wave_compile_features})
target_compile_definitions(write-f32 PRIVATE ${wave_compile_definitions})
target_compile_options(write-f32 PRIVATE
    ${wave_c_flags}
    $<$<CONFIG:RELEASE>:${wave_compile_options_release}>
    $<$<CONFIG:RELWITHDEBINFO>:${wave_compile_options_release}>
    )
