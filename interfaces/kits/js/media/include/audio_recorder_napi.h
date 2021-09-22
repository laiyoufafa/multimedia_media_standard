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

#ifndef AUDIO_RECORDER_NAPI_H_
#define AUDIO_RECORDER_NAPI_H_

#include "recorder.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
enum JSAudioSourceType : int32_t {
    JS_MIC = 1,
};

enum JSAudioEncoder : int32_t {
    JS_AAC_LC = 1,
};

enum JSFileFormat : int32_t {
    JS_MP4 = 1,
    JS_M4A = 2,
};

class AudioRecorderNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateAudioRecorder(napi_env env, napi_callback_info info);
    static napi_value Prepare(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Resume(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Reset(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    void ErrorCallback(MediaServiceExtErrCode errCode);
    void StateCallback(const std::string &callbackName);

    struct AudioRecorderProperties {
        AudioRecorderProperties();
        ~AudioRecorderProperties();
        AudioSourceType sourceType;
        OutputFormatType outputFormatType;
        AudioCodecFormat audioCodecFormat;
        int32_t encodeBitRate;
        int32_t audioSampleRate;
        int32_t numberOfChannels;
    };
    int32_t GetAudioProperties(napi_env env, napi_value args, AudioRecorderProperties &properties);
    int32_t GetAudioUriPath(napi_env env, napi_value args, std::string &uriPath);
    int32_t OnPrepare(const std::string &uriPath, const AudioRecorderProperties &properties);
    int32_t SetUri(const std::string &uriPath);
    int32_t CheckValidPath(const std::string &filePath, std::string &realPath);
    AudioRecorderNapi();
    ~AudioRecorderNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Recorder> recorderImpl_ = nullptr;
    std::shared_ptr<RecorderCallback> callbackNapi_ = nullptr;
    std::unique_ptr<TaskQueue> taskQue_;
};
}  // namespace Media
}  // namespace OHOS
#endif // AUDIO_RECORDER_NAPI_H_
