/*
 * Copyright (C) 2017 Twilio, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "com_twilio_video_ConnectOptions.h"
#include "com_twilio_video_IceOptions.h"
#include "com_twilio_video_PlatformInfo.h"
#include "com_twilio_video_LocalAudioTrack.h"
#include "com_twilio_video_LocalVideoTrack.h"
#include "com_twilio_video_EncodingParameters.h"

#include "webrtc/sdk/android/src/jni/jni_helpers.h"
#include "class_reference_holder.h"

namespace twilio_video_jni {

twilio::media::AudioCodec getAudioCodec(JNIEnv *env, jobject j_audio_codec) {
    jclass j_audio_codec_class = webrtc_jni::GetObjectClass(env, j_audio_codec);
    jmethodID j_name_method_id = webrtc_jni::GetMethodID(env,
                                                         j_audio_codec_class,
                                                         "name",
                                                         "()Ljava/lang/String;");
    jstring j_audio_codec_name = (jstring) env->CallObjectMethod(j_audio_codec, j_name_method_id);
    CHECK_EXCEPTION(env) << "Failed to get name of audio codec";
    std::string audio_codec_name = webrtc_jni::JavaToStdString(env, j_audio_codec_name);
    twilio::media::AudioCodec audio_codec;

    if (audio_codec_name == "ISAC") {
        audio_codec = twilio::media::AudioCodec::ISAC;
    } else if (audio_codec_name == "OPUS") {
        audio_codec = twilio::media::AudioCodec::OPUS;
    } else if (audio_codec_name == "PCMA") {
        audio_codec = twilio::media::AudioCodec::PCMA;
    } else if (audio_codec_name == "PCMU") {
        audio_codec = twilio::media::AudioCodec::PCMU;
    } else if (audio_codec_name == "G722") {
        audio_codec = twilio::media::AudioCodec::G722;
    } else {
        FATAL() << "Failed to get native audio codec for " << audio_codec_name;
    }

    return audio_codec;
}

twilio::media::VideoCodec getVideoCodec(JNIEnv *env, jobject j_video_codec) {
    jclass j_video_codec_class = webrtc_jni::GetObjectClass(env, j_video_codec);
    jmethodID j_name_method_id = webrtc_jni::GetMethodID(env,
                                                         j_video_codec_class,
                                                         "name",
                                                         "()Ljava/lang/String;");
    jstring j_video_codec_name = (jstring) env->CallObjectMethod(j_video_codec, j_name_method_id);
    CHECK_EXCEPTION(env) << "Failed to get name of video codec";
    std::string video_codec_name = webrtc_jni::JavaToStdString(env, j_video_codec_name);
    twilio::media::VideoCodec video_codec;

    if (video_codec_name == "H264") {
        video_codec = twilio::media::VideoCodec::H264;
    } else if (video_codec_name == "VP8") {
        video_codec = twilio::media::VideoCodec::VP8;
    } else if (video_codec_name == "VP9") {
        video_codec = twilio::media::VideoCodec::VP9;
    } else {
        FATAL() << "Failed to get native video codec for " << video_codec_name;
    }

    return video_codec;
}

JNIEXPORT jlong JNICALL
Java_com_twilio_video_ConnectOptions_nativeCreate(JNIEnv *env,
                                                  jobject j_instance,
                                                  jstring j_access_token,
                                                  jstring j_room_name,
                                                  jobjectArray j_audio_tracks,
                                                  jobjectArray j_video_tracks,
                                                  jobject j_ice_options,
                                                  jboolean j_enable_insights,
                                                  jlong j_platform_info_handle,
                                                  jobjectArray j_preferred_audio_codecs,
                                                  jobjectArray j_preferred_video_codecs,
                                                  jobject j_encoding_parameters) {

    std::string access_token = webrtc_jni::JavaToStdString(env, j_access_token);
    twilio::video::ConnectOptions::Builder* builder =
        new twilio::video::ConnectOptions::Builder(access_token);

    if (!webrtc_jni::IsNull(env, j_room_name)) {
        std::string name = webrtc_jni::JavaToStdString(env, j_room_name);
        builder->setRoomName(name);
    }

    if (!webrtc_jni::IsNull(env, j_audio_tracks)) {
        jclass j_local_audio_track_class =
            twilio_video_jni::FindClass(env, "com/twilio/video/LocalAudioTrack");
        jmethodID j_local_audio_track_get_native_handle =
            webrtc_jni::GetMethodID(env, j_local_audio_track_class, "getNativeHandle", "()J");

        std::vector<std::shared_ptr<twilio::media::LocalAudioTrack>> audio_tracks;
        int size = env->GetArrayLength(j_audio_tracks);
        if (size > 0) {
            for (int i = 0; i < size; i++) {
                // Get local audio track handle
                jobject j_audio_track = (jobject) env->GetObjectArrayElement(j_audio_tracks, i);
                jlong j_audio_track_handle =
                    (jlong) env->CallLongMethod(j_audio_track,
                                                j_local_audio_track_get_native_handle);
                // Get local audio track
                auto audio_track = getLocalAudioTrack(j_audio_track_handle);
                audio_tracks.push_back(audio_track);
            }
            builder->setAudioTracks(audio_tracks);
        }
    }

    if(!webrtc_jni::IsNull(env, j_video_tracks)) {
        jclass j_local_video_track_class =
            twilio_video_jni::FindClass(env, "com/twilio/video/LocalVideoTrack");
        jmethodID j_local_video_track_get_native_handle =
            webrtc_jni::GetMethodID(env, j_local_video_track_class, "getNativeHandle", "()J");

        std::vector<std::shared_ptr<twilio::media::LocalVideoTrack>> video_tracks;
        int size = env->GetArrayLength(j_video_tracks);
        if (size > 0) {
            for (int i = 0; i < size; i++) {
                // Get local video track handle
                jobject j_video_track = (jobject) env->GetObjectArrayElement(j_video_tracks, i);
                jlong j_video_track_handle =
                    (jlong) env->CallLongMethod(j_video_track,
                                                j_local_video_track_get_native_handle);
                // Get local video track
                auto video_track = getLocalVideoTrack(j_video_track_handle);
                video_tracks.push_back(video_track);
            }
            builder->setVideoTracks(video_tracks);
        }
    }

    if (!webrtc_jni::IsNull(env, j_preferred_audio_codecs)) {
        std::vector<twilio::media::AudioCodec> preferred_audio_codecs;
        int size = env->GetArrayLength(j_preferred_audio_codecs);
        if (size > 0) {
            for (int i = 0; i < size; i++) {
                jobject j_audio_codec =
                        (jobject) env->GetObjectArrayElement(j_preferred_audio_codecs, i);
                twilio::media::AudioCodec audio_codec = getAudioCodec(env, j_audio_codec);
                preferred_audio_codecs.push_back(audio_codec);
            }
            builder->setPreferredAudioCodecs(preferred_audio_codecs);
        }
    }

    if (!webrtc_jni::IsNull(env, j_preferred_video_codecs)) {
        std::vector<twilio::media::VideoCodec> preferred_video_codecs;
        int size = env->GetArrayLength(j_preferred_video_codecs);
        if (size > 0) {
            for (int i = 0; i < size; i++) {
                jobject j_video_codec =
                        (jobject) env->GetObjectArrayElement(j_preferred_video_codecs, i);
                twilio::media::VideoCodec video_codec = getVideoCodec(env, j_video_codec);
                preferred_video_codecs.push_back(video_codec);
            }
            builder->setPreferredVideoCodecs(preferred_video_codecs);
        }
    }

    if (!webrtc_jni::IsNull(env, j_ice_options)) {
        twilio::media::IceOptions ice_options = IceOptions::getIceOptions(env, j_ice_options);
        builder->setIceOptions(ice_options);
    }

    if (!webrtc_jni::IsNull(env, j_encoding_parameters)) {
        twilio::media::EncodingParameters encoding_parameters =
                getEncodingParameters(env, j_encoding_parameters);
        builder->setEncodingParameters(encoding_parameters);
    }

    PlatformInfoContext *platform_info_context =
        reinterpret_cast<PlatformInfoContext *>(j_platform_info_handle);
    if (platform_info_context != nullptr) {
        builder->setPlatformInfo(platform_info_context->platform_info);
    }

    builder->enableInsights(j_enable_insights);

    return webrtc_jni::jlongFromPointer(builder);
}

}
