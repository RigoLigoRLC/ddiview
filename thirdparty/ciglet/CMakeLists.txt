
add_library(ciglet STATIC
            ciglet.c
            external/fast_median.c
            external/fftsg_h.c
            external/wavfile.c)

target_compile_definitions(ciglet PRIVATE -DFP_TYPE=double)

target_include_directories(ciglet PUBLIC inc)
target_include_directories(ciglet PRIVATE external)
