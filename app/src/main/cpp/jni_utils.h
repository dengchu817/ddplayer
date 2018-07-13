//
// Created by 邓楚 on 2018/6/27.
//

#ifndef SIMPLE_PLAYER_JNIUTILS_H
#define SIMPLE_PLAYER_JNIUTILS_H

#include <jni.h>

struct fields_t{
    jfieldID context;
    jmethodID postEventFromNativeID;
};

#endif //SIMPLE_PLAYER_JNIUTILS_H
