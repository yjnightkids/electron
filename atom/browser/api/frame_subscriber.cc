// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include "atom/common/native_mate_converters/gfx_converter.h"
#include "base/bind.h"
#include "content/public/browser/render_widget_host.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace {

void CopyPixelsToBuffer(const SkBitmap& bitmap,
                        const v8::Local<v8::Object>& buffer) {
  size_t rgb_arr_size = bitmap.width() * bitmap.height() *
      bitmap.bytesPerPixel();

  memcpy(node::Buffer::Data(buffer), bitmap.getPixels(), rgb_arr_size);
}
  
}  // namespace

namespace api {

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 content::RenderWidgetHostView* view,
                                 const FrameCaptureCallback& callback,
                                 bool only_dirty)
    : isolate_(isolate),
      view_(view),
      callback_(callback),
      only_dirty_(only_dirty),
      source_id_for_copy_request_(base::UnguessableToken::Create()),
      weak_factory_(this) {
}

bool FrameSubscriber::ShouldCaptureFrame(
    const gfx::Rect& dirty_rect,
    base::TimeTicks present_time,
    scoped_refptr<media::VideoFrame>* storage,
    DeliverFrameCallback* callback) {
  if (!view_)
    return false;

  if (dirty_rect.IsEmpty())
    return false;

  gfx::Rect rect = gfx::Rect(view_->GetVisibleViewportSize());
  if (only_dirty_)
    rect = dirty_rect;

  gfx::Size view_size = rect.size();
  gfx::Size bitmap_size = view_size;
  gfx::NativeView native_view = view_->GetNativeView();
  const float scale =
      display::Screen::GetScreen()->GetDisplayNearestView(native_view)
      .device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ScaleToCeiledSize(view_size, scale);

  rect = gfx::Rect(rect.origin(), bitmap_size);

  view_->CopyFromSurface(
      rect,
      rect.size(),
      base::Bind(&FrameSubscriber::OnFrameDelivered,
                 weak_factory_.GetWeakPtr(), callback_, rect),
      kBGRA_8888_SkColorType);

  return false;
}

const base::UnguessableToken& FrameSubscriber::GetSourceIdForCopyRequest() {
  return source_id_for_copy_request_;
}

void FrameSubscriber::OnFrameDelivered(const FrameCaptureCallback& callback,
                                       const gfx::Rect& damage_rect,
                                       const SkBitmap& bitmap,
                                       content::ReadbackResponse response) {
  if (response != content::ReadbackResponse::READBACK_SUCCESS)
    return;

  v8::Locker locker(isolate_);
  v8::HandleScope handle_scope(isolate_);

  size_t rgb_arr_size = bitmap.width() * bitmap.height() *
    bitmap.bytesPerPixel();
  v8::MaybeLocal<v8::Object> buffer = node::Buffer::New(isolate_, rgb_arr_size);
  if (buffer.IsEmpty())
    return;

  CopyPixelsToBuffer(bitmap, buffer.ToLocalChecked());

  v8::Local<v8::Value> damage =
      mate::Converter<gfx::Rect>::ToV8(isolate_, damage_rect);

  callback_.Run(buffer.ToLocalChecked(), damage);
}

}  // namespace api

}  // namespace atom
