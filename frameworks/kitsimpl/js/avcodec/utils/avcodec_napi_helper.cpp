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

#include "avcodec_napi_helper.h"

namespace OHOS {
namespace Media {
bool AVCodecNapiHelper::IsEos()
{
    if (isEos_.load()) {
        return true;
    }
    return false;
}

void AVCodecNapiHelper::SetEos(bool eos)
{
    isEos_.store(eos);
}

bool AVCodecNapiHelper::IsStop()
{
    if (isStop_.load()) {
        return true;
    }
    return false;
}

bool AVCodecNapiHelper::IsFlushing()
{
    return isFlushing_.load();
}

void AVCodecNapiHelper::SetStop(bool stop)
{
    isStop_.store(stop);
}

void AVCodecNapiHelper::SetFlushing(bool flushing)
{
    isFlushing_.store(flushing);
}
}
}
