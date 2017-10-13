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

#include "android_room_observer.h"

#include "video/room.h"
#include "class_reference_holder.h"
#include "logging.h"
#include "jni_utils.h"

namespace twilio_video_jni {

AndroidRoomObserver::AndroidRoomObserver(JNIEnv *env,
                                         jobject j_room,
                                         jobject j_room_observer,
                                         jobject j_connect_options,
                                         jobject j_handler) :
        j_room_(env, j_room),
        j_room_observer_(env, j_room_observer),
        j_connect_options_(env, j_connect_options),
        j_handler_(env, j_handler),
        j_room_class_(env, webrtc_jni::GetObjectClass(env, *j_room_)),
        j_room_observer_class_(env, webrtc_jni::GetObjectClass(env, *j_room_observer_)),
        j_local_participant_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/LocalParticipant")),
        j_twilio_exception_class_(env, twilio_video_jni::FindClass(env,
                                                                   "com/twilio/video/TwilioException")),
        j_remote_participant_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/RemoteParticipant")),
        j_array_list_class_(env, twilio_video_jni::FindClass(env, "java/util/ArrayList")),
        j_published_audio_track_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/LocalAudioTrackPublication")),
        j_remote_audio_track_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/RemoteAudioTrack")),
        j_remote_audio_track_publication_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/RemoteAudioTrackPublication")),
        j_published_video_track_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/LocalVideoTrackPublication")),
        j_remote_video_track_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/RemoteVideoTrack")),
        j_remote_video_track_publication_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/RemoteVideoTrackPublication")),
        j_published_data_track_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/LocalDataTrackPublication")),
        j_remote_data_track_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/RemoteDataTrack")),
        j_remote_data_track_publication_class_(env, twilio_video_jni::FindClass(env, "com/twilio/video/RemoteDataTrackPublication")),
        j_set_connected_(
                webrtc_jni::GetMethodID(env,
                                        *j_room_class_,
                                        "setConnected",
                                        "(Ljava/lang/String;Lcom/twilio/video/LocalParticipant;Ljava/util/List;)V")),
        j_on_connected_(
                webrtc_jni::GetMethodID(env,
                                        *j_room_observer_class_,
                                        "onConnected",
                                        "(Lcom/twilio/video/Room;)V")),
        j_on_disconnected_(
                webrtc_jni::GetMethodID(env,
                                        *j_room_observer_class_,
                                        "onDisconnected",
                                        "(Lcom/twilio/video/Room;Lcom/twilio/video/TwilioException;)V")),
        j_on_connect_failure_(
                webrtc_jni::GetMethodID(env,
                                        *j_room_observer_class_,
                                        "onConnectFailure",
                                        "(Lcom/twilio/video/Room;Lcom/twilio/video/TwilioException;)V")),
        j_on_participant_connected_(
                webrtc_jni::GetMethodID(env,
                                        *j_room_observer_class_,
                                        "onParticipantConnected",
                                        "(Lcom/twilio/video/Room;Lcom/twilio/video/RemoteParticipant;)V")),
        j_on_participant_disconnected_(
                webrtc_jni::GetMethodID(env,
                                        *j_room_observer_class_,
                                        "onParticipantDisconnected",
                                        "(Lcom/twilio/video/Room;Lcom/twilio/video/RemoteParticipant;)V")),
        j_on_recording_started_(
                webrtc_jni::GetMethodID(env,
                                        *j_room_observer_class_,
                                        "onRecordingStarted",
                                        "(Lcom/twilio/video/Room;)V")),
        j_on_recording_stopped_(
                webrtc_jni::GetMethodID(env,
                                        *j_room_observer_class_,
                                        "onRecordingStopped",
                                        "(Lcom/twilio/video/Room;)V")),
        j_local_participant_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_local_participant_class_,
                                        "<init>",
                                        kLocalParticipantConstructorSignature)),
        j_participant_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_remote_participant_class_,
                                        "<init>",
                                        "(Ljava/lang/String;Ljava/lang/String;Ljava/util/List;Ljava/util/List;Ljava/util/List;Landroid/os/Handler;J)V")),
        j_array_list_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_array_list_class_,
                                        "<init>",
                                        "()V")),
        j_array_list_add_(
                webrtc_jni::GetMethodID(env,
                                        *j_array_list_class_,
                                        "add",
                                        "(Ljava/lang/Object;)Z")),
        j_published_audio_track_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_published_audio_track_class_,
                                        "<init>",
                                        "(Ljava/lang/String;Lcom/twilio/video/LocalAudioTrack;)V")),
        j_audio_track_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_remote_audio_track_class_,
                                        "<init>",
                                        "(Lorg/webrtc/AudioTrack;Ljava/lang/String;Z)V")),
        j_audio_track_publication_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_remote_audio_track_publication_class_,
                                        "<init>",
                                        "(ZZLjava/lang/String;Ljava/lang/String;)V")),
        j_published_video_track_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_published_video_track_class_,
                                        "<init>",
                                        "(Ljava/lang/String;Lcom/twilio/video/LocalVideoTrack;)V")),
        j_video_track_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_remote_video_track_class_,
                                        "<init>",
                                        "(Lorg/webrtc/VideoTrack;Ljava/lang/String;Z)V")),
        j_video_track_publication_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_remote_video_track_publication_class_,
                                        "<init>",
                                        "(ZZLjava/lang/String;Ljava/lang/String;)V")),
        j_data_track_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_remote_data_track_class_,
                                        "<init>",
                                        "(ZZZIILjava/lang/String;J)V")),
        j_published_data_track_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_published_data_track_class_,
                                        "<init>",
                                        "(Ljava/lang/String;Lcom/twilio/video/LocalDataTrack;)V")),
        j_data_track_publication_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_remote_data_track_publication_class_,
                                        "<init>",
                                        "(ZZLjava/lang/String;Ljava/lang/String;)V")),
        j_connect_options_get_audio_tracks_(
                webrtc_jni::GetMethodID(env,
                                        webrtc_jni::GetObjectClass(env, *j_connect_options_),
                                        "getAudioTracks",
                                        "()Ljava/util/List;")),
        j_connect_options_get_video_tracks_(
                webrtc_jni::GetMethodID(env,
                                        webrtc_jni::GetObjectClass(env, *j_connect_options_),
                                        "getVideoTracks",
                                        "()Ljava/util/List;")),
        j_connect_options_get_data_tracks_(
                webrtc_jni::GetMethodID(env,
                                        webrtc_jni::GetObjectClass(env, *j_connect_options_),
                                        "getDataTracks",
                                        "()Ljava/util/List;")),
        j_twilio_exception_ctor_id_(
                webrtc_jni::GetMethodID(env,
                                        *j_twilio_exception_class_,
                                        "<init>",
                                        "(ILjava/lang/String;Ljava/lang/String;)V")) {
    VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                      twilio::video::LogLevel::kDebug,
                      "AndroidRoomObserver");
}

AndroidRoomObserver::~AndroidRoomObserver()  {
    VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                      twilio::video::LogLevel::kDebug,
                      "~AndroidRoomObserver");

    // Delete all remaining global references to Participants
    for (auto it = remote_participants_.begin() ; it != remote_participants_.end() ; it++) {
        webrtc_jni::DeleteGlobalRef(jni(), it->second);
    }
    remote_participants_.clear();
}

void AndroidRoomObserver::setObserverDeleted()  {
    rtc::CritScope cs(&deletion_lock_);
    observer_deleted_ = true;
    VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                      twilio::video::LogLevel::kDebug,
                      "room observer deleted");
}

void AndroidRoomObserver::onConnected(twilio::video::Room *room) {
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

        jstring j_room_sid = JavaUTF16StringFromStdString(jni(), room->getSid());
        std::shared_ptr<twilio::video::LocalParticipant> local_participant =
                room->getLocalParticipant();
        jobject j_local_audio_tracks = getLocalAudioTracks();
        jobject j_local_video_tracks = getLocalVideoTracks();
        jobject j_local_data_tracks = getLocalDataTracks();
        jobject j_local_participant = createJavaLocalParticipant(jni(),
                                                                 local_participant,
                                                                 *j_local_participant_class_,
                                                                 j_local_participant_ctor_id_,
                                                                 j_local_audio_tracks,
                                                                 j_local_video_tracks,
                                                                 j_local_data_tracks,
                                                                 *j_array_list_class_,
                                                                 j_array_list_ctor_id_,
                                                                 j_array_list_add_,
                                                                 *j_published_audio_track_class_,
                                                                 j_published_audio_track_ctor_id_,
                                                                 *j_published_video_track_class_,
                                                                 j_published_video_track_ctor_id_,
                                                                 *j_published_data_track_class_,
                                                                 j_published_data_track_ctor_id_,
                                                                 *j_handler_);
        jobject j_participants = createJavaParticipantList(room->getRemoteParticipants());
        jni()->CallVoidMethod(*j_room_,
                              j_set_connected_,
                              j_room_sid,
                              j_local_participant,
                              j_participants);
        CHECK_EXCEPTION(jni()) << "error calling setConnected";

        jni()->CallVoidMethod(*j_room_observer_,
                              j_on_connected_,
                              *j_room_);
        CHECK_EXCEPTION(jni()) << "error calling onConnected";
    }
}

void AndroidRoomObserver::onDisconnected(const twilio::video::Room *room,
                                         std::unique_ptr<twilio::video::TwilioError> twilio_error) {
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

        jobject j_twilio_exception = nullptr;
        if (twilio_error != nullptr) {
            j_twilio_exception = createJavaRoomException(*twilio_error);
        }
        jni()->CallVoidMethod(*j_room_observer_, j_on_disconnected_, *j_room_, j_twilio_exception);
        CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
    }
}

void AndroidRoomObserver::onConnectFailure(const twilio::video::Room *room,
                                           const twilio::video::TwilioError twilio_error) {
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

        jobject j_room_exception = createJavaRoomException(twilio_error);
        jni()->CallVoidMethod(*j_room_observer_, j_on_connect_failure_, *j_room_, j_room_exception);
        CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
    }
}

void AndroidRoomObserver::onParticipantConnected(twilio::video::Room *room,
                                                 std::shared_ptr<twilio::video::RemoteParticipant> participant) {
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

        // Create participant
        jobject j_participant = createJavaRemoteParticipant(jni(),
                                                            participant,
                                                            *j_remote_participant_class_,
                                                            j_participant_ctor_id_,
                                                            *j_array_list_class_,
                                                            j_array_list_ctor_id_,
                                                            j_array_list_add_,
                                                            *j_remote_audio_track_class_,
                                                            *j_remote_audio_track_publication_class_,
                                                            j_audio_track_ctor_id_,
                                                            j_audio_track_publication_ctor_id_,
                                                            *j_remote_video_track_class_,
                                                            *j_remote_video_track_publication_class_,
                                                            j_video_track_ctor_id_,
                                                            j_video_track_publication_ctor_id_,
                                                            *j_remote_data_track_class_,
                                                            *j_remote_data_track_publication_class_,
                                                            j_data_track_ctor_id_,
                                                            j_data_track_publication_ctor_id_,
                                                            *j_handler_);
        // Add global ref to java participant so we can reference in onParticipantDisconnected
        remote_participants_.insert(std::make_pair(participant,
                                                   webrtc_jni::NewGlobalRef(jni(), j_participant)));

        jni()->CallVoidMethod(*j_room_observer_,
                              j_on_participant_connected_,
                              *j_room_,
                              j_participant);
        CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
    }
}

void AndroidRoomObserver::onParticipantDisconnected(twilio::video::Room *room,
                                                    std::shared_ptr<twilio::video::RemoteParticipant> participant)  {
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

        auto it = remote_participants_.find(participant);
        jobject j_participant = it->second;
        jni()->CallVoidMethod(*j_room_observer_,
                              j_on_participant_disconnected_,
                              *j_room_,
                              j_participant);
        CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";

        // Remove participant and delete global reference after developer is notified
        remote_participants_.erase(it);
        webrtc_jni::DeleteGlobalRef(jni(), j_participant);
    }
}

void AndroidRoomObserver::onRecordingStarted(twilio::video::Room *room) {
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

        jni()->CallVoidMethod(*j_room_observer_,
                              j_on_recording_started_,
                              *j_room_);

        CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
    }
}

void AndroidRoomObserver::onRecordingStopped(twilio::video::Room *room)  {
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

        jni()->CallVoidMethod(*j_room_observer_,
                              j_on_recording_stopped_,
                              *j_room_);

        CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
    }
}

JNIEnv* AndroidRoomObserver::jni()  {
    return webrtc_jni::AttachCurrentThreadIfNeeded();
}

bool AndroidRoomObserver::isObserverValid(const std::string &callbackName) {
    if (observer_deleted_) {
        VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                          twilio::video::LogLevel::kWarning,
                          "room observer is marked for deletion, skipping %s callback",
                          callbackName.c_str());
        return false;
    };
    if (webrtc_jni::IsNull(jni(), *j_room_observer_)) {
        VIDEO_ANDROID_LOG(twilio::video::LogModule::kPlatform,
                          twilio::video::LogLevel::kWarning,
                          "room observer reference has been destroyed, skipping %s callback",
                          callbackName.c_str());
        return false;
    }
    return true;
}

jobject AndroidRoomObserver::createJavaRoomException(
        const twilio::video::TwilioError &twilio_error) {

    return jni()->NewObject(*j_twilio_exception_class_,
                            j_twilio_exception_ctor_id_,
                            twilio_error.getCode(),
                            JavaUTF16StringFromStdString(jni(),
                                                                twilio_error.getMessage()),
                            JavaUTF16StringFromStdString(jni(),
                                                                twilio_error.getExplanation()));
}

jobject AndroidRoomObserver::createJavaParticipantList(
        const std::map<std::string, std::shared_ptr<twilio::video::RemoteParticipant>> participants) {

    // Create ArrayList<RemoteParticipant>
    jobject j_participants = jni()->NewObject(*j_array_list_class_, j_array_list_ctor_id_);

    std::map<std::string, std::shared_ptr<twilio::video::RemoteParticipant>>::const_iterator it;
    for (it = participants.begin(); it != participants.end(); ++it) {
        std::shared_ptr<twilio::video::RemoteParticipant> participant = it->second;
        jobject j_participant = createJavaRemoteParticipant(jni(),
                                                            participant,
                                                            *j_remote_participant_class_,
                                                            j_participant_ctor_id_,
                                                            *j_array_list_class_,
                                                            j_array_list_ctor_id_,
                                                            j_array_list_add_,
                                                            *j_remote_audio_track_class_,
                                                            *j_remote_audio_track_publication_class_,
                                                            j_audio_track_ctor_id_,
                                                            j_audio_track_publication_ctor_id_,
                                                            *j_remote_video_track_class_,
                                                            *j_remote_video_track_publication_class_,
                                                            j_video_track_ctor_id_,
                                                            j_video_track_publication_ctor_id_,
                                                            *j_remote_data_track_class_,
                                                            *j_remote_data_track_publication_class_,
                                                            j_data_track_ctor_id_,
                                                            j_data_track_publication_ctor_id_,
                                                            *j_handler_);
        // Add global ref to java participant so we can reference in onParticipantDisconnected
        remote_participants_.insert(std::make_pair(participant, webrtc_jni::NewGlobalRef(jni(),
                                                                                         j_participant)));
        jni()->CallBooleanMethod(j_participants, j_array_list_add_, j_participant);
    }
    return j_participants;
}

jobject AndroidRoomObserver::getLocalAudioTracks() {
    jobject j_local_audio_tracks = jni()->CallObjectMethod(*j_connect_options_,
                                                           j_connect_options_get_audio_tracks_);
    CHECK_EXCEPTION(jni()) << "Failed to get local audio tracks";

    return j_local_audio_tracks;
}

jobject AndroidRoomObserver::getLocalVideoTracks() {
    jobject j_local_video_tracks = jni()->CallObjectMethod(*j_connect_options_,
                                                           j_connect_options_get_video_tracks_);
    CHECK_EXCEPTION(jni()) << "Failed to get local video tracks";

    return j_local_video_tracks;
}

jobject AndroidRoomObserver::getLocalDataTracks() {
    jobject j_local_data_tracks = jni()->CallObjectMethod(*j_connect_options_,
                                                           j_connect_options_get_data_tracks_);
    CHECK_EXCEPTION(jni()) << "Failed to get local data tracks";

    return j_local_data_tracks;
}

}
