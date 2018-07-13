cmake_minimum_required(VERSION 3.4.1)

SET ( sdl_lib_shared ijksdl )

add_library(ijkffmpeg
            SHARED
            IMPORTED)
set_target_properties(ijkffmpeg
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../distribution/ffmpeg/lib/${ANDROID_ABI}/libijkffmpeg.so)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../distribution/ffmpeg/include)

add_library(ijkj4a
            SHARED
            IMPORTED)
set_target_properties(ijkj4a
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../distribution/ijkj4a/lib/${ANDROID_ABI}/libijkj4a.so)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ijkj4a)

add_library(ijkyuv
            SHARED
            IMPORTED)
set_target_properties(ijkyuv
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../distribution/ijkyuv/lib/${ANDROID_ABI}/libijkyuv.so)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ijkyuv/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

add_library(${sdl_lib_shared}
            SHARED
            ijksdl_aout.c
            ijksdl_audio.c
            ijksdl_egl.c
            ijksdl_error.c
            ijksdl_mutex.c
            ijksdl_stdinc.c
            ijksdl_thread.c
            ijksdl_timer.c
            ijksdl_vout.c
            ijksdl_extra_log.c
            gles2/color.c
            gles2/common.c
            gles2/renderer.c
            gles2/renderer_rgb.c
            gles2/renderer_yuv420p.c
            gles2/renderer_yuv444p10le.c
            gles2/shader.c
            gles2/fsh/rgb.fsh.c
            gles2/fsh/yuv420p.fsh.c
            gles2/fsh/yuv444p10le.fsh.c
            gles2/vsh/mvp.vsh.c

            dummy/ijksdl_vout_dummy.c

            ffmpeg/ijksdl_vout_overlay_ffmpeg.c
            ffmpeg/abi_all/image_convert.c

            android/android_audiotrack.c
            android/android_nativewindow.c
            android/ijksdl_android_jni.c
            android/ijksdl_aout_android_audiotrack.c
            android/ijksdl_aout_android_opensles.c
            android/ijksdl_codec_android_mediacodec_dummy.c
            android/ijksdl_codec_android_mediacodec_internal.c
            android/ijksdl_codec_android_mediacodec_java.c
            android/ijksdl_codec_android_mediacodec.c
            android/ijksdl_codec_android_mediadef.c
            android/ijksdl_codec_android_mediaformat_java.c
            android/ijksdl_codec_android_mediaformat.c
            android/ijksdl_vout_android_nativewindow.c
            android/ijksdl_vout_android_surface.c
            android/ijksdl_vout_overlay_android_mediacodec.c

            )

set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../distribution/ijksdl)

set_target_properties(${sdl_lib_shared}
                      PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY
                      "${distribution_DIR}/lib/${ANDROID_ABI}")

target_link_libraries( # Specifies the target library.
                      ${sdl_lib_shared}
                       log
                       android
                       OpenSLES
                       EGL
                       GLESv2
                       ijkyuv
                       ijkj4a
                       ijkffmpeg
                       )