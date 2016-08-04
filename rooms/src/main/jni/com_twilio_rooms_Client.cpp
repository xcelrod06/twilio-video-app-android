#include "com_twilio_rooms_Client.h"
#include "webrtc/api/java/jni/jni_helpers.h"



#include "webrtc/base/refcount.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/modules/video_capture/video_capture_internal.h"
#include "webrtc/api/java/jni/androidvideocapturer_jni.h"
#include "webrtc/modules/audio_device/android/audio_manager.h"
#include "webrtc/modules/audio_device/android/opensles_player.h"
#include "webrtc/api/java/jni/classreferenceholder.h"
#include "webrtc/media/engine/webrtcvideodecoderfactory.h"

#include "AccessManager/AccessManager.h"
#include "TSCLogger.h"
#include "video.h"
#include "media/media_factory.h"
//#include "ITSCVideoCodec.h"
//#include "TSCMediaCodecRegistry.h"
#include "android_platform_info_provider.h"
#include "android_video_codec_manager.h"
#include "android_room_observer.h"
#include "connect_options.h"

#include <memory>

using namespace twiliosdk;
using namespace webrtc_jni;

static bool media_jvm_set = false;

extern "C" jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    std::string func_name = std::string(__FUNCTION__);
    TS_CORE_LOG_MODULE(kTSCoreLogModulePlatform, kTSCoreLogLevelDebug, "%s", func_name.c_str());
    jint ret = InitGlobalJniVariables(jvm);
    if (ret < 0) {
        TS_CORE_LOG_MODULE(kTSCoreLogModulePlatform,
                           kTSCoreLogLevelError,
                           "InitGlobalJniVariables() failed");
        return -1;
    }
    webrtc_jni::LoadGlobalClassReferenceHolder();

    return ret;
}

extern "C" void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM *jvm, void *reserved) {
    std::string func_name = std::string(__FUNCTION__);
    TS_CORE_LOG_MODULE(kTSCoreLogModulePlatform, kTSCoreLogLevelDebug, "%s", func_name.c_str());
    webrtc_jni::FreeGlobalClassReferenceHolder();
}

JNIEXPORT void JNICALL Java_com_twilio_rooms_RoomsClient_nativeSetCoreLogLevel
    (JNIEnv *env, jobject instance, jint level) {
    TS_CORE_LOG_MODULE(kTSCoreLogModulePlatform, kTSCoreLogLevelDebug, "setCoreLogLevel");
    TSCoreLogLevel coreLogLevel = static_cast<TSCoreLogLevel>(level);
    TSCLogger::instance()->setLogLevel(coreLogLevel);
}

JNIEXPORT void JNICALL Java_com_twilio_rooms_RoomsClient_nativeSetModuleLevel
    (JNIEnv *env, jobject instance, jint module, jint level) {
    TS_CORE_LOG_MODULE(kTSCoreLogModulePlatform, kTSCoreLogLevelDebug, "setModuleLevel");
    TSCoreLogModule coreLogModule = static_cast<TSCoreLogModule>(module);
    TSCoreLogLevel coreLogLevel = static_cast<TSCoreLogLevel>(level);
    TSCLogger::instance()->setModuleLogLevel(coreLogModule, coreLogLevel);
}

JNIEXPORT jint JNICALL Java_com_twilio_rooms_RoomsClient_nativeGetCoreLogLevel
    (JNIEnv *env, jobject instance) {
    TS_CORE_LOG_MODULE(kTSCoreLogModulePlatform, kTSCoreLogLevelDebug, "getCoreLogLevel");
    return TSCLogger::instance()->getLogLevel();
}

JNIEXPORT jboolean JNICALL
Java_com_twilio_rooms_RoomsClient_nativeInitialize(JNIEnv *env, jobject instance, jobject context) {
    bool failure = false;

    // Setup media related Android device objects
    if (!media_jvm_set) {
        failure |= webrtc::OpenSLESPlayer::SetAndroidAudioDeviceObjects(GetJVM(), context);
        failure |= webrtc::VoiceEngine::SetAndroidObjects(GetJVM(), context);
        failure |= webrtc_jni::AndroidVideoCapturerJni::SetAndroidObjects(env, context);
        media_jvm_set = true;
    }

    return failure ? JNI_FALSE : JNI_TRUE;
}

class ClientDataHolder {
public:
    ClientDataHolder(std::unique_ptr<twilio::video::Client> client,
                     AndroidRoomObserver* android_room_observer,
                     std::shared_ptr<TwilioCommon::AccessManager> access_manager//,
                     /*std::shared_ptr<media::MediaStack> media_stack*/) {
        client_ = std::move(client);
        android_room_observer_ = android_room_observer;
        access_manager_ = access_manager;
        //media_stack_ = media_stack;
    }

private:
    std::unique_ptr<twilio::video::Client> client_;
    AndroidRoomObserver* android_room_observer_;
    std::shared_ptr<TwilioCommon::AccessManager> access_manager_;
    //std::shared_ptr<media::MediaStack> media_stack_;

};

JNIEXPORT jlong JNICALL
Java_com_twilio_rooms_RoomsClient_nativeConnect(JNIEnv *env,
                                                jobject instance,
                                                jobject context,
                                                jstring tokenString,
                                                jlong android_room_observer_handle,
                                                jstring name) {
    // Lazily initialize the core relying on a disconnect as the condition used to destroy the core
    //twiliosdk::TSCSDK *sdk = new twiliosdk::TSCSDK();

    AndroidPlatformInfoProvider provider(env, context);
    //sdk->setPlatformInfo(provider.getReport());

//    TSCVideoCodecRef androidVideoCodecManager =
//        new rtc::RefCountedObject<AndroidVideoCodecManager>();
    //TSCMediaCodecRegistry &codecManager = sdk->getMediaCodecRegistry();
    //codecManager.registerVideoCodec(androidVideoCodecManager);

    // Connect to a room
    std::string token = webrtc_jni::JavaToStdString(env, tokenString);
    std::shared_ptr<TwilioCommon::AccessManager>
        access_manager = TwilioCommon::AccessManager::create(
        token, nullptr);

    AndroidRoomObserver *android_room_observer =
        reinterpret_cast<AndroidRoomObserver *>(android_room_observer_handle);
    //std::shared_ptr<twilio::video::RoomObserver> room_observer(android_room_observer);

    //std::shared_ptr<media::MediaStack> media_stack(new media::MediaStack());
    //media_stack->start(media::MediaStackOptions());
    std::shared_ptr<twilio::media::MediaFactory> media_factory =
        twilio::media::MediaFactory::create(twilio::media::MediaOptions());

    twilio::video::Invoker *invoker = new twilio::video::Invoker();

    std::unique_ptr<twilio::video::Client> client =
        twilio::video::Client::create(access_manager,
                                      //client_observer,
                                      media_factory,
                                      invoker);

    if(IsNull(env, name)) {
        std::shared_ptr<twilio::video::RoomFuture> future = client->connect(android_room_observer);
    } else {
        std::string roomName = webrtc_jni::JavaToStdString(env, name);
        twilio::video::ConnectOptions connect_options = twilio::video::ConnectOptions::Builder()
                .setRoomName(roomName)
                .setCreateRoom(true)
                .build();
        std::shared_ptr<twilio::video::RoomFuture> future =
            client->connect(connect_options, android_room_observer);
    }

    // TODO: properly delete holder
    //new ClientDataHolder(std::move(client), client_observer, access_manager, media_stack);
    new ClientDataHolder(std::move(client), android_room_observer, access_manager);//, media_stack);

    return 0;
}

JNIEXPORT jlong JNICALL
Java_com_twilio_rooms_RoomsClient_00024RoomListenerHandle_nativeCreate(JNIEnv *env,
                                                                       jobject instance,
                                                                       jobject object) {
    TS_CORE_LOG_MODULE(kTSCoreLogModulePlatform, kTSCoreLogLevelDebug,
                       "Create AndroidRoomObserver");
    AndroidRoomObserver *androidRoomObserver = new AndroidRoomObserver(env, object);
    return jlongFromPointer(androidRoomObserver);
}

JNIEXPORT void JNICALL
Java_com_twilio_rooms_RoomsClient_00024RoomListenerHandle_nativeFree(JNIEnv *env,
                                                                     jobject instance,
                                                                     jlong nativeHandle) {
    TS_CORE_LOG_MODULE(kTSCoreLogModulePlatform, kTSCoreLogLevelDebug,
                       "Free AndroidRoomObserver");
    AndroidRoomObserver
        *androidRoomObserver = reinterpret_cast<AndroidRoomObserver *>(nativeHandle);
    if (androidRoomObserver != nullptr) {
        androidRoomObserver->setObserverDeleted();
        delete androidRoomObserver;
    }
}
