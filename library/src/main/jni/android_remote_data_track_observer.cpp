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

#include "android_remote_data_track_observer.h"
#include "logging.h"
#include "class_reference_holder.h"
#include "jni_utils.h"

namespace twilio_video_jni {

AndroidRemoteDataTrackObserver::AndroidRemoteDataTrackObserver(JNIEnv *env,
                                                               jobject j_remote_data_track,
                                                               jobject j_remote_data_track_listener)
        : j_remote_data_track_(env, j_remote_data_track),
          j_remote_data_track_listener_(env, j_remote_data_track_listener),
          j_remote_data_track_listener_class_(env,
                                              webrtc_jni::GetObjectClass(env, j_remote_data_track_listener)),
          j_byte_buffer_class_(env, twilio_video_jni::FindClass(env, "java/nio/ByteBuffer")),
          j_on_string_message_(webrtc_jni::GetMethodID(env,
                                                       *j_remote_data_track_listener_class_,
                                                       "onMessage",
                                                       "(Lcom/twilio/video/RemoteDataTrack;Ljava/lang/String;)V")),
          j_on_buffer_message_(webrtc_jni::GetMethodID(env,
                                                       *j_remote_data_track_listener_class_,
                                                       "onMessage",
                                                       "(Lcom/twilio/video/RemoteDataTrack;Ljava/nio/ByteBuffer;)V")),
          j_byte_buffer_wrap_id_(webrtc_jni::GetStaticMethodID(env,
                                                               *j_byte_buffer_class_,
                                                               "wrap",
                                                               "([B)Ljava/nio/ByteBuffer;")) {

}

AndroidRemoteDataTrackObserver::~AndroidRemoteDataTrackObserver() {
    VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                      twilio::video::LogLevel::kDebug,
                      "~AndroidRemoteDataTrackObserver");
}

void AndroidRemoteDataTrackObserver::setObserverDeleted() {
    rtc::CritScope cs(&deletion_lock_);
    observer_deleted_ = true;
    VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                      twilio::video::LogLevel::kDebug,
                      "local participant observer deleted");
}

void twilio_video_jni::AndroidRemoteDataTrackObserver::onMessage(twilio::media::RemoteDataTrack *remote_data_track,
                                                                 const std::string &message) {
    webrtc_jni::ScopedLocalRefFrame local_ref_frame(jni());
    std::string func_name = std::string(__FUNCTION__);
    VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                      twilio::video::LogLevel::kDebug,
                      "%s", func_name.c_str());
    {
        rtc::CritScope cs(&deletion_lock_);

        if (!isObserverValid(func_name)) {
            return;
        }

        jni()->CallVoidMethod(*j_remote_data_track_listener_,
                              j_on_string_message_,
                              *j_remote_data_track_,
                              JavaUTF16StringFromStdString(jni(), message));
        CHECK_EXCEPTION(jni()) << "Error calling onMessage(String)";
    }
}

void twilio_video_jni::AndroidRemoteDataTrackObserver::onMessage(twilio::media::RemoteDataTrack *track,
                                                                 const uint8_t *message,
                                                                 size_t size) {
    webrtc_jni::ScopedLocalRefFrame local_ref_frame(jni());
    std::string func_name = std::string(__FUNCTION__);
    VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                      twilio::video::LogLevel::kDebug,
                      "%s", func_name.c_str());
    {
        rtc::CritScope cs(&deletion_lock_);

        if (!isObserverValid(func_name)) {
            return;
        }

        // Convert message to ByteBuffer
        jbyteArray message_bytes = jni()->NewByteArray((jsize) size);
        jni()->SetByteArrayRegion(message_bytes, 0,
                                  (jsize) size,
                                  reinterpret_cast<const jbyte*>(message));
        jobject message_buffer = jni()->CallStaticObjectMethod(*j_byte_buffer_class_,
                                                               j_byte_buffer_wrap_id_,
                                                               message_bytes);

        // Invoke callback
        jni()->CallVoidMethod(*j_remote_data_track_listener_,
                              j_on_buffer_message_,
                              *j_remote_data_track_,
                              message_buffer);
        CHECK_EXCEPTION(jni()) << "Error calling onMessage(Buffer)";
    }
}

bool AndroidRemoteDataTrackObserver::isObserverValid(const std::string &callback_name) {
    if (observer_deleted_) {
        VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                          twilio::video::LogLevel::kWarning,
                          "remote data track listener is marked for deletion, skipping %s callback",
                          callback_name.c_str());
        return false;
    };
    if (webrtc_jni::IsNull(jni(), *j_remote_data_track_listener_)) {
        VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                          twilio::video::LogLevel::kWarning,
                          "remote data track listener reference has been destroyed, "
                                  "skipping %s callback",
                          callback_name.c_str());
        return false;
    }
    return true;
}

}
