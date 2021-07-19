/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "player_service_proxy.h"
#include "player_listener_stub.h"
#include "media_log.h"
#include "errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServiceProxy"};
}

namespace OHOS {
namespace Media {
PlayerServiceProxy::PlayerServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardPlayerService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServiceProxy::~PlayerServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destory", FAKE_POINTER(this));
}

int32_t PlayerServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(SET_LISTENER_OBJ, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set listener obj failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::SetSource(const std::string &uri)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteString(uri);
    int error = Remote()->SendRequest(SET_SOURCE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set Source failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::Play()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(PLAY, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Play failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::Prepare()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(PREPARE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Prepare failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::PrepareAsync()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(PREPAREASYNC, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("PrepareAsync failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::Pause()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(PAUSE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Pause failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(STOP, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Stop failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::Reset()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(RESET, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Reset failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(RELEASE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Release failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::SetVolume(float leftVolume, float rightVolume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteFloat(leftVolume);
    data.WriteFloat(rightVolume);
    int error = Remote()->SendRequest(SET_VOLUME, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set volume failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::Seek(uint64_t mSeconds, int32_t mode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteUint64(mSeconds);
    data.WriteInt32(mode);
    int error = Remote()->SendRequest(SEEK, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Seek failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::GetCurrentTime(uint64_t &currentTime)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(GET_CURRENT_TIME, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Get current time failed, error: %{public}d", error);
        return error;
    }
    currentTime = reply.ReadUint64();
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::GetDuration(uint64_t &duration)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(GET_DURATION, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Get current time failed, error: %{public}d", error);
        return error;
    }
    duration = reply.ReadUint64();
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::SetPlaybackSpeed(int32_t mode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(mode);
    int error = Remote()->SendRequest(SET_PLAYERBACK_SPEED, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("SetPlaybackSpeed failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::GetPlaybackSpeed(int32_t &mode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(GET_PLAYERBACK_SPEED, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("GetPlaybackSpeed failed, error: %{public}d", error);
        return error;
    }
    mode = reply.ReadInt32();
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::SetVideoSurface(sptr<Surface> surface)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(surface != nullptr, ERR_INVALID_VALUE, "surface is nullptr");
    sptr<IBufferProducer> producer = surface->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, ERR_INVALID_VALUE, "producer is nullptr");

    sptr<IRemoteObject> object = producer->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_VALUE, "object is nullptr");

    const std::string SURFACE_FORMAT = "SURFACE_FORMAT";
    std::string format = surface->GetUserData(SURFACE_FORMAT);
    MEDIA_LOGI("surface_format is %{public}s!", format.c_str());
    data.WriteRemoteObject(object);
    data.WriteString(format);
    int error = Remote()->SendRequest(SET_VIDEO_SURFACE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set video surface failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

bool PlayerServiceProxy::IsPlaying()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(IS_PLAYING, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Get is playing failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadBool();
}

bool PlayerServiceProxy::IsLooping()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(IS_LOOPING, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Get is looping failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadBool();
}

int32_t PlayerServiceProxy::SetLooping(bool loop)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteBool(loop);
    int error = Remote()->SendRequest(SET_LOOPING, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set looping failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(DESTROY, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("destroy failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t PlayerServiceProxy::SetPlayerCallback()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(SET_CALLBACK, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("set callback failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}
} // namespace Media
} // namespace OHOS