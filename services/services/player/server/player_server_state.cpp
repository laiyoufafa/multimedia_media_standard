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

#include "player_server_state.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerState"};
    enum PlayerTaskID : int32_t {
        PREPARE_TASK_ID = 0,
        PLAY_TASK_ID = 1,
        PAUSE_TASK_ID = 2,
        STOP_TASK_ID = 3,
        SEEK_TASK_ID = 4,
    };
}

namespace OHOS {
namespace Media {
void PlayerServer::BaseState::ReportInvalidOperation()
{
    MEDIA_LOGE("invalid operation for %{public}s", GetStateName().c_str());
    server_.OnError(PLAYER_ERROR, MSERR_INVALID_STATE);
}

int32_t PlayerServer::BaseState::Prepare()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::Play()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::Pause()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    (void)mSeconds;
    (void)mode;

    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::Stop()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    (void)mode;

    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

void PlayerServer::BaseState::OnMessageReceived(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    MEDIA_LOGD("message received, type = %{public}d, extra = %{public}d", type, extra);
    (void)infoBody;
    if (type == INFO_TYPE_SEEKDONE || type == INFO_TYPE_SPEEDDONE) {
        if (type == INFO_TYPE_SEEKDONE) {
            MediaTrace::TraceEnd("Player::Seek", SEEK_TASK_ID);
        }
        (void)server_.taskMgr_.MarkTaskDone();
        return;
    }

    if (type == INFO_TYPE_STATE_CHANGE) {
        if (extra == PLAYER_PLAYBACK_COMPLETE) {
            HandlePlaybackComplete(extra);
        } else {
            HandleStateChange(extra);
            BehaviorEventWrite(server_.GetStatusDescription(extra).c_str(), "Player");
            MEDIA_LOGI("Callback State change, currentState is %{public}s",
                server_.GetStatusDescription(extra).c_str());
        }
    }
}

void PlayerServer::IdleState::StateEnter()
{
    (void)server_.HandleReset();
}

int32_t PlayerServer::InitializedState::Prepare()
{
    server_.ChangeState(server_.preparingState_);
    return MSERR_OK;
}

void PlayerServer::PreparingState::StateEnter()
{
    (void)server_.HandlePrepare();
    MEDIA_LOGD("PlayerServer::PreparingState::StateEnter finished");
}

int32_t PlayerServer::PreparingState::Stop()
{
    server_.ChangeState(server_.stoppedState_);
    return MSERR_OK;
}

void PlayerServer::PreparingState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_PREPARED) {
        MediaTrace::TraceEnd("PlayerServer::PrepareAsync", PREPARE_TASK_ID);
        server_.ChangeState(server_.preparedState_);
        server_.stateCond_.notify_one(); // awake the stateCond_'s waiter in Prepare()
        (void)server_.taskMgr_.MarkTaskDone();
    }
}

int32_t PlayerServer::PreparedState::Prepare()
{
    return MSERR_OK;
}

int32_t PlayerServer::PreparedState::Play()
{
    return server_.HandlePlay();
}

int32_t PlayerServer::PreparedState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    return server_.HandleSeek(mSeconds, mode);
}

int32_t PlayerServer::PreparedState::Stop()
{
    return server_.HandleStop();
}

int32_t PlayerServer::PreparedState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    return server_.HandleSetPlaybackSpeed(mode);
}

void PlayerServer::PreparedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        MediaTrace::TraceEnd("PlayerServer::Play", PLAY_TASK_ID);
        server_.ChangeState(server_.playingState_);
        server_.startTimeMonitor_.FinishTime();
        (void)server_.taskMgr_.MarkTaskDone();
    } else if (newState == PLAYER_STOPPED) {
        MediaTrace::TraceEnd("PlayerServer::Stop", STOP_TASK_ID);
        server_.ChangeState(server_.stoppedState_);
        (void)server_.taskMgr_.MarkTaskDone();
    }
}

int32_t PlayerServer::PlayingState::Play()
{
    return MSERR_OK;
}

int32_t PlayerServer::PlayingState::Pause()
{
    return server_.HandlePause();
}

int32_t PlayerServer::PlayingState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    return server_.HandleSeek(mSeconds, mode);
}

int32_t PlayerServer::PlayingState::Stop()
{
    return server_.HandleStop();
}

int32_t PlayerServer::PlayingState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    return server_.HandleSetPlaybackSpeed(mode);
}

void PlayerServer::PlayingState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_PAUSED) {
        MediaTrace::TraceEnd("PlayerServer::Pause", PAUSE_TASK_ID);
        server_.ChangeState(server_.pausedState_);
        (void)server_.taskMgr_.MarkTaskDone();
    } else if (newState == PLAYER_STOPPED) {
        MediaTrace::TraceEnd("PlayerServer::Stop", STOP_TASK_ID);
        server_.ChangeState(server_.stoppedState_);
        (void)server_.taskMgr_.MarkTaskDone();
    }
}

void PlayerServer::PlayingState::HandlePlaybackComplete(int32_t extra)
{
    server_.lastOpStatus_ = static_cast<PlayerStates>(extra);
    server_.ChangeState(server_.playbackCompletedState_);
}

int32_t PlayerServer::PausedState::Play()
{
    return server_.HandlePlay();
}

int32_t PlayerServer::PausedState::Pause()
{
    return MSERR_OK;
}

int32_t PlayerServer::PausedState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    return server_.HandleSeek(mSeconds, mode);
}

int32_t PlayerServer::PausedState::Stop()
{
    return server_.HandleStop();
}

int32_t PlayerServer::PausedState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    return server_.HandleSetPlaybackSpeed(mode);
}

void PlayerServer::PausedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        MediaTrace::TraceEnd("PlayerServer::Play", PLAY_TASK_ID);
        server_.ChangeState(server_.playingState_);
        (void)server_.taskMgr_.MarkTaskDone();
    } else if (newState == PLAYER_STOPPED) {
        MediaTrace::TraceEnd("PlayerServer::Stop", STOP_TASK_ID);
        server_.ChangeState(server_.stoppedState_);
        (void)server_.taskMgr_.MarkTaskDone();
    }
}

int32_t PlayerServer::StoppedState::Prepare()
{
    server_.ChangeState(server_.preparingState_);
    return MSERR_OK;
}

int32_t PlayerServer::StoppedState::Stop()
{
    return MSERR_OK;
}

int32_t PlayerServer::PlaybackCompletedState::Play()
{
    return server_.HandlePlay();
}

int32_t PlayerServer::PlaybackCompletedState::Stop()
{
    return server_.HandleStop();
}

void PlayerServer::PlaybackCompletedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        MediaTrace::TraceEnd("PlayerServer::Play", PLAY_TASK_ID);
        server_.ChangeState(server_.playingState_);
        (void)server_.taskMgr_.MarkTaskDone();
    } else if (newState == PLAYER_STOPPED) {
        MediaTrace::TraceEnd("PlayerServer::Stop", STOP_TASK_ID);
        server_.ChangeState(server_.stoppedState_);
        (void)server_.taskMgr_.MarkTaskDone();
    }
}
}
}