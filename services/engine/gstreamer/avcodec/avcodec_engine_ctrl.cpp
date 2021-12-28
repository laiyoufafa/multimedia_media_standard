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

#include "avcodec_engine_ctrl.h"
#include <vector>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecEngineCtrl"};
}

namespace OHOS {
namespace Media {
AVCodecEngineCtrl::AVCodecEngineCtrl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecEngineCtrl::~AVCodecEngineCtrl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    (void)Release();
    src_ = nullptr;
    sink_ = nullptr;
    if (codecBin_ != nullptr) {
        gst_object_unref(codecBin_);
        codecBin_ = nullptr;
    }
    if (gstPipeline_ != nullptr) {
        gst_object_unref(gstPipeline_);
        gstPipeline_ = nullptr;
    }
    if (bus_ != nullptr) {
        g_clear_object(&bus_);
        bus_ = nullptr;
    }
}

int32_t AVCodecEngineCtrl::Init(AVCodecType type, bool useSoftware, const std::string &name)
{
    MEDIA_LOGD("Enter Init");
    codecType_ = type;
    gstPipeline_ = GST_PIPELINE_CAST(gst_pipeline_new("codec-pipeline"));
    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_NO_MEMORY);

    bus_ = gst_pipeline_get_bus(gstPipeline_);
    CHECK_AND_RETURN_RET(bus_ != nullptr, MSERR_UNKNOWN);
    gst_bus_set_sync_handler(bus_, BusSyncHandler, this, nullptr);

    codecBin_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("codecbin", "the_codec_bin")));
    CHECK_AND_RETURN_RET(codecBin_ != nullptr, MSERR_NO_MEMORY);

    gboolean ret = gst_bin_add(GST_BIN_CAST(gstPipeline_), codecBin_);
    CHECK_AND_RETURN_RET(ret == TRUE, MSERR_NO_MEMORY);

    g_object_set(codecBin_, "use-software", static_cast<gboolean>(useSoftware), nullptr);
    g_object_set(codecBin_, "type", static_cast<int32_t>(type), nullptr);
    g_object_set(codecBin_, "coder-name", name.c_str(), nullptr);

    isEncoder_ = (type == AVCODEC_TYPE_VIDEO_ENCODER) || (type == AVCODEC_TYPE_AUDIO_ENCODER);
    if (isEncoder_) {
        g_object_set(codecBin_, "src-convert", static_cast<gboolean>(true), nullptr);
    } else {
        g_object_set(codecBin_, "sink-convert", static_cast<gboolean>(true), nullptr);
    }

    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Prepare(std::shared_ptr<ProcessorConfig> inputConfig,
    std::shared_ptr<ProcessorConfig> outputConfig)
{
    if (src_ == nullptr) {
        MEDIA_LOGD("Use buffer src");
        src_ = AVCodecEngineFactory::CreateSrc(SrcType::SRC_TYPE_BYTEBUFFER);
        CHECK_AND_RETURN_RET_LOG(src_ != nullptr, MSERR_NO_MEMORY, "No memory");
        CHECK_AND_RETURN_RET(src_->Init() == MSERR_OK, MSERR_UNKNOWN);
        CHECK_AND_RETURN_RET(src_->SetCallback(obs_) == MSERR_OK, MSERR_UNKNOWN);
    }

    if (sink_ == nullptr) {
        MEDIA_LOGD("Use buffer sink");
        sink_ = AVCodecEngineFactory::CreateSink(SinkType::SINK_TYPE_BYTEBUFFER);
        CHECK_AND_RETURN_RET_LOG(sink_ != nullptr, MSERR_NO_MEMORY, "No memory");
        CHECK_AND_RETURN_RET(sink_->Init() == MSERR_OK, MSERR_UNKNOWN);
        CHECK_AND_RETURN_RET(sink_->SetCallback(obs_) == MSERR_OK, MSERR_UNKNOWN);
    }

    CHECK_AND_RETURN_RET(codecBin_ != nullptr, MSERR_UNKNOWN);
    if (inputConfig->needParser_) {
        g_object_set(codecBin_, "parser", static_cast<gboolean>(true), nullptr);
    }
    g_object_set(codecBin_, "src", static_cast<gpointer>(src_->GetElement()), nullptr);
    CHECK_AND_RETURN_RET(src_->Configure(inputConfig) == MSERR_OK, MSERR_UNKNOWN);

    if (useSurfaceRender_) {
        MEDIA_LOGD("In surface mode, we need to overwrite pixel format, using RGBA instead");
        GstCaps *caps = outputConfig->caps_;
        CHECK_AND_RETURN_RET(caps != nullptr, MSERR_UNKNOWN);
        GValue value = G_VALUE_INIT;
        g_value_init (&value, G_TYPE_STRING);
        g_value_set_string (&value, "RGBA");
        gst_caps_set_value(caps, "format", &value);
        g_value_unset(&value);
    }

    g_object_set(codecBin_, "sink", static_cast<gpointer>(sink_->GetElement()), nullptr);
    CHECK_AND_RETURN_RET(sink_->Configure(outputConfig) == MSERR_OK, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_UNKNOWN);
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(gstPipeline_), GST_STATE_PAUSED);
    CHECK_AND_RETURN_RET(ret != GST_STATE_CHANGE_FAILURE, MSERR_UNKNOWN);
    if (ret == GST_STATE_CHANGE_ASYNC) {
        MEDIA_LOGD("Wait state change");
        std::unique_lock<std::mutex> lock(gstPipeMutex_);
        gstPipeCond_.wait(lock);
    }

    MEDIA_LOGD("Prepare success");
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Start()
{
    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_UNKNOWN);
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(gstPipeline_), GST_STATE_PLAYING);
    CHECK_AND_RETURN_RET(ret != GST_STATE_CHANGE_FAILURE, MSERR_UNKNOWN);
    if (ret == GST_STATE_CHANGE_ASYNC) {
        MEDIA_LOGD("Wait state change");
        std::unique_lock<std::mutex> lock(gstPipeMutex_);
        gstPipeCond_.wait(lock);
    }
    if (needInputCallback_ && isFirstStart) {
        auto obs = obs_.lock();
        CHECK_AND_RETURN_RET(obs != nullptr, MSERR_UNKNOWN);
        CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
        uint32_t bufferCount = src_->GetBufferCount();
        for (uint32_t i = 0; i < bufferCount; i++) {
            obs->OnInputBufferAvailable(i);
        }
        isFirstStart = false;
    }

    MEDIA_LOGD("Start success");
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Stop()
{
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(gstPipeline_), GST_STATE_PAUSED);
    CHECK_AND_RETURN_RET(ret != GST_STATE_CHANGE_FAILURE, MSERR_UNKNOWN);
    if (ret == GST_STATE_CHANGE_ASYNC) {
        std::unique_lock<std::mutex> lock(gstPipeMutex_);
        gstPipeCond_.wait(lock);
    }

    MEDIA_LOGD("Stop success");
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Flush()
{
    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(src_->Flush() == MSERR_OK, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(sink_->Flush() == MSERR_OK, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(codecBin_ != nullptr, MSERR_UNKNOWN);
    GstEvent *event = gst_event_new_flush_start();
    CHECK_AND_RETURN_RET(event != nullptr, MSERR_NO_MEMORY);
    (void)gst_element_send_event(codecBin_, event);

    event = gst_event_new_flush_stop(FALSE);
    CHECK_AND_RETURN_RET(event != nullptr, MSERR_NO_MEMORY);
    (void)gst_element_send_event(codecBin_, event);

    if (needInputCallback_) {
        auto obs = obs_.lock();
        CHECK_AND_RETURN_RET(obs != nullptr, MSERR_UNKNOWN);
        uint32_t bufferCount = src_->GetBufferCount();
        for (uint32_t i = 0; i < bufferCount; i++) {
            obs->OnInputBufferAvailable(i);
        }
    }

    MEDIA_LOGD("Flush success");
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Release()
{
    if (gstPipeline_ != nullptr) {
        (void)gst_element_set_state(GST_ELEMENT_CAST(gstPipeline_), GST_STATE_NULL);
    }
    return MSERR_OK;
}

void AVCodecEngineCtrl::SetObs(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    obs_ = obs;
}

sptr<Surface> AVCodecEngineCtrl::CreateInputSurface(std::shared_ptr<ProcessorConfig> inputConfig)
{
    CHECK_AND_RETURN_RET(codecType_ == AVCODEC_TYPE_VIDEO_ENCODER, nullptr);
    if (src_ == nullptr) {
        MEDIA_LOGD("Use surface src");
        src_ = AVCodecEngineFactory::CreateSrc(SrcType::SRC_TYPE_SURFACE);
        CHECK_AND_RETURN_RET_LOG(src_ != nullptr, nullptr, "No memory");
        CHECK_AND_RETURN_RET_LOG(src_->Init() == MSERR_OK, nullptr, "Failed to create input surface");
    }
    auto surface =  src_->CreateInputSurface(inputConfig);
    CHECK_AND_RETURN_RET(surface != nullptr, nullptr);
    needInputCallback_ = false;
    useSurfaceInput_ = true;

    MEDIA_LOGD("CreateInputSurface success");
    return surface;
}

int32_t AVCodecEngineCtrl::SetOutputSurface(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET(codecType_ == AVCODEC_TYPE_VIDEO_DECODER, MSERR_INVALID_OPERATION);
    if (sink_ == nullptr) {
        MEDIA_LOGD("Use surface sink");
        sink_ = AVCodecEngineFactory::CreateSink(SinkType::SINK_TYPE_SURFACE);
        CHECK_AND_RETURN_RET_LOG(sink_ != nullptr, MSERR_NO_MEMORY, "No memory");
        CHECK_AND_RETURN_RET(sink_->Init() == MSERR_OK, MSERR_UNKNOWN);
        CHECK_AND_RETURN_RET(sink_->SetCallback(obs_) == MSERR_OK, MSERR_UNKNOWN);
    }
    if (sink_->SetOutputSurface(surface) == MSERR_OK) {
        useSurfaceRender_ = true;
    } else {
        return MSERR_UNKNOWN;
    }

    MEDIA_LOGD("SetOutputSurface success");
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVCodecEngineCtrl::GetInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET(src_ != nullptr, nullptr);
    return src_->GetInputBuffer(index);
}

int32_t AVCodecEngineCtrl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    int32_t ret = src_->QueueInputBuffer(index, info, flag);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    bufferCount_++;
    if (flag == AVCODEC_BUFFER_FLAG_EOS) {
        CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
        MEDIA_LOGI("EOS frame, index:%{public}d", bufferCount_);
        sink_->SetEOS(bufferCount_);
    }
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVCodecEngineCtrl::GetOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET(sink_ != nullptr, nullptr);
    return sink_->GetOutputBuffer(index);
}

int32_t AVCodecEngineCtrl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
    return sink_->ReleaseOutputBuffer(index, render);
}

int32_t AVCodecEngineCtrl::SetParameter(const Format &format)
{
    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(src_->SetParameter(format) == MSERR_OK, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(sink_->SetParameter(format) == MSERR_OK, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(codecBin_ != nullptr, MSERR_UNKNOWN);
    int32_t value = 0;
    if (format.GetIntValue("req_i_frame", value) == true) {
        g_object_set(codecBin_, "req-i-frame", value, nullptr);
    }

    MEDIA_LOGD("SetParameter success");
    return MSERR_OK;
}

GstBusSyncReply AVCodecEngineCtrl::BusSyncHandler(GstBus *bus, GstMessage *message, gpointer userData)
{
    CHECK_AND_RETURN_RET(message != nullptr, GST_BUS_DROP);

    auto self = reinterpret_cast<AVCodecEngineCtrl *>(userData);
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_STATE_CHANGED: {
            CHECK_AND_RETURN_RET(message->src != nullptr, GST_BUS_DROP);
            if (GST_IS_BIN(message->src) && !GST_IS_PIPELINE(message->src)) {
                MEDIA_LOGD("Finish state change");
                std::unique_lock<std::mutex> lock(self->gstPipeMutex_);
                self->gstPipeCond_.notify_all();
            }
            break;
        }
        case GST_MESSAGE_ERROR: {
            int32_t errCode = MSERR_UNKNOWN;
            GError *err = nullptr;
            gst_message_parse_error(message, &err, nullptr);
            if (err->domain == GST_CORE_ERROR) {
                errCode = MSERR_UNKNOWN;
            } else if (err->domain == GST_LIBRARY_ERROR) {
                errCode = MSERR_UNSUPPORT;
            } else
            if (err->domain == GST_RESOURCE_ERROR) {
                errCode = MSERR_INVALID_VAL;
            } else if (err->domain == GST_STREAM_ERROR) {
                errCode = MSERR_DATA_SOURCE_ERROR_UNKNOWN;
            }

            auto obs = self->obs_.lock();
            CHECK_AND_RETURN_RET(obs != nullptr, GST_BUS_DROP);
            obs->OnError(AVCODEC_ERROR_INTERNAL, errCode);
            break;
        }
        default: {
            break;
        }
    }
    return GST_BUS_PASS;
}
} // Media
} // OHOS