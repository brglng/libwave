add_executable(write-f32 main.c)
target_link_libraries(write-f32 wav::wav)
target_include_directories(write-f32 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(write-f32 PRIVATE ${wav_compile_features})
target_compile_definitions(write-f32 PRIVATE ${wav_compile_definitions})
target_compile_options(write-f32 PRIVATE
    ${wav_c_flags}
    $<$<CONFIG:RELEASE>:${wav_compile_options_release}>
    $<$<CONFIG:RELWITHDEBINFO>:${wav_compile_options_release}>
    )
