/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "nocopyable.h"
#include "media_errors.h"
#include "avcodec_common.h"
#include "vdec_mock.h"
using namespace std;
using namespace OHOS::Media::VCodecTestParam;
namespace OHOS {
namespace Media {
VDecCallbackTest::VDecCallbackTest(std::shared_ptr<VDecSignal> signal)
    : signal_(signal)
{
}

VDecCallbackTest::~VDecCallbackTest()
{
}

void VDecCallbackTest::OnError(int32_t errorCode)
{
    cout << "VDec Error errorCode=" << errorCode << endl;
}

void VDecCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    cout << "VDec Format Changed" << endl;
}

void VDecCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
{
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inIndexQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void VDecCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr)
{
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outIndexQueue_.push(index);

    signal_->outSizeQueue_.push(attr.size);
    signal_->outCond_.notify_all();
}

VDecMock::VDecMock(std::shared_ptr<VDecSignal> signal)
    : signal_(signal)
{
}

VDecMock::~VDecMock()
{
}

bool VDecMock::CreateVideoDecMockByMine(const std::string &mime)
{
    videoDec_ = AVCodecMockFactory::CreateVideoDecMockByMine(mime);
    return videoDec_ != nullptr;
}

bool VDecMock::CreateVideoDecMockByName(const std::string &name)
{
    videoDec_ = AVCodecMockFactory::CreateVideoDecMockByName(name);
    return videoDec_ != nullptr;
}

int32_t VDecMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    return videoDec_->SetCallback(cb);
}

int32_t VDecMock::SetOutputSurface(std::shared_ptr<SurfaceMock> surface)
{
    return videoDec_->SetOutputSurface(surface);
}

int32_t VDecMock::Configure(std::shared_ptr<FormatMock> format)
{
    return videoDec_->Configure(format);
}

int32_t VDecMock::Prepare()
{
    return videoDec_->Prepare();
}

int32_t VDecMock::Start()
{
    isRunning_.store(true);

    testFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, MSERR_OK, "Fatal: No memory");
    testFile_->open("/data/test/out_320_240_10s.h264", std::ios::in | std::ios::binary);

    inputLoop_ = make_unique<thread>(&VDecMock::InpLoopFunc, this);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, MSERR_OK, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&VDecMock::OutLoopFunc, this);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, MSERR_OK, "Fatal: No memory");
    return videoDec_->Start();
}

int32_t VDecMock::Stop()
{
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->inMutex_);
        signal_->inIndexQueue_.push(10000);  // push 10000 to stop queue
        signal_->inCond_.notify_all();
        queueLock.unlock();
        inputLoop_->join();
        inputLoop_.reset();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outIndexQueue_.push(10000); // push 10000 to stop queue
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_.reset();
    }
    return videoDec_->Stop();
}

int32_t VDecMock::Flush()
{
    return videoDec_->Flush();
}

int32_t VDecMock::Reset()
{
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->inMutex_);
        signal_->inIndexQueue_.push(10000); // push 10000 to stop queue
        signal_->inCond_.notify_all();
        queueLock.unlock();
        inputLoop_->join();
        inputLoop_.reset();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outIndexQueue_.push(10000);  // push 10000 to stop queue
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_.reset();
    }
    return videoDec_->Reset();
}

int32_t VDecMock::Release()
{
    return videoDec_->Release();
}

std::shared_ptr<FormatMock> VDecMock::GetOutputMediaDescription()
{
    return videoDec_->GetOutputMediaDescription();
}

int32_t VDecMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    return videoDec_->SetParameter(format);
}

int32_t VDecMock::PushInputData(uint32_t index, AVCodecBufferAttrMock &attr)
{
    return videoDec_->PushInputData(index, attr);
}

int32_t VDecMock::RenderOutputData(uint32_t index)
{
    return videoDec_->RenderOutputData(index);
}

int32_t VDecMock::FreeOutputData(uint32_t index)
{
    return videoDec_->FreeOutputData(index);
}

int32_t VDecMock::PushInputDataMock(uint32_t index)
{
    struct AVCodecBufferAttrMock attr;
    attr.offset = 0;
    if (frameCount_ == ES_LENGTH) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        attr.size = 0;
        attr.pts = 0;
        cout << "EOS Frame, frameCount = " << frameCount_ << endl;
        isRunning_.store(false);
    } else {
        if (isFirstFrame_) {
            attr.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
            isFirstFrame_ = false;
        } else {
            attr.flags = AVCODEC_BUFFER_FLAG_NONE;
        }
        attr.size = bufferSize;
        attr.pts = timestamp_;
    }
    return videoDec_->PushInputData(index, attr);
}

void VDecMock::InpLoopFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return signal_->inIndexQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inIndexQueue_.front();
        std::shared_ptr<AVMemoryMock> buffer = signal_->inBufferQueue_.front();
        UNITTEST_CHECK_AND_RETURN_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        UNITTEST_CHECK_AND_RETURN_LOG(testFile_ != nullptr && testFile_->is_open(), "Fatal: open file fail");

        uint32_t bufferSize = 0;

        if (frameCount_ < ES_LENGTH) {
            bufferSize =  ES[frameCount_]; // replace with the actual size
            char *fileBuffer = (char *)malloc(sizeof(char) * bufferSize + 1);
            UNITTEST_CHECK_AND_RETURN_LOG(fileBuffer != nullptr, "Fatal: malloc fail.");
            (void)testFile_->read(fileBuffer, bufferSize);
            if (testFile_->eof()) {
                cout << "Finish" << endl;
                free(fileBuffer);
                break;
            }

            if (memcpy_s(buffer->GetAddr(), buffer->GetSize(), fileBuffer, bufferSize) != EOK) {
                cout << "Fatal: memcpy fail" << endl;
                free(fileBuffer);
                break;
            }
            free(fileBuffer);
        }
        if (videoDec_->PushInputDataMock(index) != MSERR_OK) {
            cout << "Fatal: PushInputData fail, exit" << endl;
        }
        timestamp_ += FRAME_DURATION_US;
        frameCount_++;
        signal_->inIndexQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
}

void VDecMock::OutLoopFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return signal_->outIndexQueue_.size() > 0; });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->outIndexQueue_.front();

        if (videoDec_->RenderOutputData(index) != MSERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail index" << index << endl;
            break;
        }
        signal_->outIndexQueue_.pop();
        signal_->outSizeQueue_.pop();
    }
}
}  // namespace Media
}  // namespace OHOS
