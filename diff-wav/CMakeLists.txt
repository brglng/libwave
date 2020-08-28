add_executable(diff-wav
    src/diff-wav.c
)

target_link_libraries(diff-wav
    wav
    $<$<PLATFORM_ID:Linux>:m>
)
