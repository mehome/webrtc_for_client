/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/desktop_capture/win/dxgi_frame.h"

#include <string.h>

#include <utility>

#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/win/dxgi_duplicator_controller.h"

namespace webrtc {

DxgiFrame::DxgiFrame(SharedMemoryFactory* factory)
    : factory_(factory) {}

DxgiFrame::~DxgiFrame() = default;

bool DxgiFrame::Prepare(DesktopSize size, DesktopCapturer::SourceId source_id) {
  if (source_id != source_id_) {
    // Once the source has been changed, the entire source should be copied.
    source_id_ = source_id;
    context_.Reset();
  }

  if (resolution_change_detector_.IsChanged(size)) {
    // Once the output size changed, recreate the SharedDesktopFrame.
    frame_.reset();
    resolution_change_detector_.Reset();
  }

  if (!frame_) {
    std::unique_ptr<DesktopFrame> frame;
    if (factory_) {
      frame = SharedMemoryDesktopFrame::Create(size, factory_);
    } else {
      frame.reset(new BasicDesktopFrame(size));
    }
    if (!frame) {
      LOG(LS_WARNING) << "DxgiFrame cannot create a new DesktopFrame.";
      return false;
    }
    // DirectX capturer won't paint each pixel in the frame due to its one
    // capturer per monitor design. So once the new frame is created, we should
    // clear it to avoid the legacy image to be remained on it. See
    // http://crbug.com/708766.
    RTC_DCHECK_EQ(frame->stride(),
                  frame->size().width() * DesktopFrame::kBytesPerPixel);
    memset(frame->data(), 0, frame->stride() * frame->size().height());

    frame_ = SharedDesktopFrame::Wrap(std::move(frame));
  }

  return !!frame_;
}

SharedDesktopFrame* DxgiFrame::frame() const {
  RTC_DCHECK(frame_);
  return frame_.get();
}

DxgiFrame::Context* DxgiFrame::context() {
  RTC_DCHECK(frame_);
  return &context_;
}

}  // namespace webrtc
