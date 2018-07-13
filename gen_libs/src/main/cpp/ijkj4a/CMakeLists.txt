cmake_minimum_required(VERSION 3.4.1)

SET ( ijkj4a_lib_shared ijkj4a )

add_library(${ijkj4a_lib_shared}
            SHARED
            j4a/j4a_allclasses.c
            j4a/j4a_base.c
            j4a/class/android/media/AudioTrack.c
            j4a/class/android/media/MediaCodec.c
            j4a/class/android/media/MediaFormat.c
            j4a/class/android/media/PlaybackParams.c
            j4a/class/android/os/Build.c
            j4a/class/android/os/Bundle.c
            j4a/class/java/nio/Buffer.c
            j4a/class/java/nio/ByteBuffer.c
            j4a/class/java/util/ArrayList.c
            j4a/class/tv/danmaku/ijk/media/player/misc/IMediaDataSource.c
            j4a/class/tv/danmaku/ijk/media/player/misc/IAndroidIO.c
            j4a/class/tv/danmaku/ijk/media/player/IjkMediaPlayer.c

            j4au/class/android/media/AudioTrack.util.c
            j4au/class/java/nio/ByteBuffer.util.c
            )


set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../distribution/ijkj4a)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${ijkj4a_lib_shared}
                      PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY
                      "${distribution_DIR}/lib/${ANDROID_ABI}")


target_link_libraries( # Specifies the target library.
                      ${ijkj4a_lib_shared}
                       # Links the target library to the log library
                       # included in the NDK.
                       log )