// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/paint_op_helper.h"
#include "cc/paint/paint_canvas.h"
#include "cc/paint/paint_op_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

TEST(PaintOpHelper, AnnotateToString) {
  AnnotateOp op(PaintCanvas::AnnotationType::URL, SkRect::MakeXYWH(1, 2, 3, 4),
                nullptr);
  op.type = static_cast<uint32_t>(AnnotateOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(
      str,
      "AnnotateOp(type=URL, rect=[1.000,2.000 3.000x4.000], data=<SkData>)");
}

TEST(PaintOpHelper, ClipPathToString) {
  ClipPathOp op(SkPath(), SkClipOp::kDifference, true);
  op.type = static_cast<uint32_t>(ClipPathOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "ClipPathOp(path=<SkPath>, op=kDifference, antialias=true)");
}

TEST(PaintOpHelper, ClipRectToString) {
  ClipRectOp op(SkRect::MakeXYWH(10.1f, 20.2f, 30.3f, 40.4f),
                SkClipOp::kIntersect, false);
  op.type = static_cast<uint32_t>(ClipRectOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str,
            "ClipRectOp(rect=[10.100,20.200 30.300x40.400], op=kIntersect, "
            "antialias=false)");
}

TEST(PaintOpHelper, ClipRRectToString) {
  ClipRRectOp op(SkRRect::MakeRect(SkRect::MakeXYWH(1, 2, 3, 4)),
                 SkClipOp::kDifference, false);
  op.type = static_cast<uint32_t>(ClipRRectOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str,
            "ClipRRectOp(rrect=[bounded by 1.000,2.000 3.000x4.000], "
            "op=kDifference, antialias=false)");
}

TEST(PaintOpHelper, ConcatToString) {
  ConcatOp op(SkMatrix::MakeAll(1, 2, 3, 4, 5, 6, 7, 8, 9));
  op.type = static_cast<uint32_t>(ConcatOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str,
            "ConcatOp(matrix=[  1.0000   2.0000   3.0000][  4.0000   5.0000   "
            "6.0000][  7.0000   8.0000   9.0000])");
}

TEST(PaintOpHelper, DrawColorToString) {
  DrawColorOp op(SkColorSetARGB(11, 22, 33, 44), SkBlendMode::kSrc);
  op.type = static_cast<uint32_t>(DrawColorOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawColorOp(color=rgba(22, 33, 44, 11), mode=kSrc)");
}

TEST(PaintOpHelper, DrawDRRectToString) {
  DrawDRRectOp op(SkRRect::MakeRect(SkRect::MakeXYWH(1, 2, 3, 4)),
                  SkRRect::MakeRect(SkRect::MakeXYWH(5, 6, 7, 8)),
                  PaintFlags());
  op.type = static_cast<uint32_t>(DrawDRRectOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str,
            "DrawDRRectOp(outer=[bounded by 1.000,2.000 3.000x4.000], "
            "inner=[bounded by 5.000,6.000 7.000x8.000])");
}

TEST(PaintOpHelper, DrawImageToString) {
  DrawImageOp op(PaintImage(), 10.5f, 20.3f, nullptr);
  op.type = static_cast<uint32_t>(DrawImageOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawImageOp(image=<paint image>, left=10.500, top=20.300)");
}

TEST(PaintOpHelper, DrawImageRectToString) {
  DrawImageRectOp op(PaintImage(), SkRect::MakeXYWH(1, 2, 3, 4),
                     SkRect::MakeXYWH(5, 6, 7, 8), nullptr,
                     PaintCanvas::kStrict_SrcRectConstraint);
  op.type = static_cast<uint32_t>(DrawImageRectOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str,
            "DrawImageRectOp(image=<paint image>, src=[1.000,2.000 "
            "3.000x4.000], dst=[5.000,6.000 7.000x8.000], "
            "constraint=kStrict_SrcRectConstraint)");
}

TEST(PaintOpHelper, DrawIRectToString) {
  DrawIRectOp op(SkIRect::MakeXYWH(1, 2, 3, 4), PaintFlags());
  op.type = static_cast<uint32_t>(DrawIRectOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawIRectOp(rect=[1,2 3x4])");
}

TEST(PaintOpHelper, DrawLineToString) {
  DrawLineOp op(1.1f, 2.2f, 3.3f, 4.4f, PaintFlags());
  op.type = static_cast<uint32_t>(DrawLineOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawLineOp(x0=1.100, y0=2.200, x1=3.300, y1=4.400)");
}

TEST(PaintOpHelper, DrawOvalToString) {
  DrawOvalOp op(SkRect::MakeXYWH(100, 200, 300, 400), PaintFlags());
  op.type = static_cast<uint32_t>(DrawOvalOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawOvalOp(oval=[100.000,200.000 300.000x400.000])");
}

TEST(PaintOpHelper, DrawPathToString) {
  SkPath path;
  DrawPathOp op(path, PaintFlags());
  op.type = static_cast<uint32_t>(DrawPathOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawPathOp(path=<SkPath>)");
}

TEST(PaintOpHelper, DrawRecordToString) {
  DrawRecordOp op(nullptr);
  op.type = static_cast<uint32_t>(DrawRecordOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawRecordOp(record=<paint record>)");
}

TEST(PaintOpHelper, DrawRectToString) {
  DrawRectOp op(SkRect::MakeXYWH(-1, -2, -3, -4), PaintFlags());
  op.type = static_cast<uint32_t>(DrawRectOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawRectOp(rect=[-1.000,-2.000 -3.000x-4.000])");
}

TEST(PaintOpHelper, DrawRRectToString) {
  DrawRRectOp op(SkRRect::MakeRect(SkRect::MakeXYWH(-1, -2, 3, 4)),
                 PaintFlags());
  op.type = static_cast<uint32_t>(DrawRRectOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "DrawRRectOp(rrect=[bounded by -1.000,-2.000 3.000x4.000])");
}

TEST(PaintOpHelper, DrawTextBlobToString) {
  DrawTextBlobOp op(nullptr, 100, -222, PaintFlags());
  op.type = static_cast<uint32_t>(DrawTextBlobOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str,
            "DrawTextBlobOp(blob=<paint text blob>, x=100.000, y=-222.000)");
}

TEST(PaintOpHelper, NoopToString) {
  NoopOp op;
  op.type = static_cast<uint32_t>(NoopOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "NoopOp()");
}

TEST(PaintOpHelper, RestoreToString) {
  RestoreOp op;
  op.type = static_cast<uint32_t>(RestoreOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "RestoreOp()");
}

TEST(PaintOpHelper, RotateToString) {
  RotateOp op(360);
  op.type = static_cast<uint32_t>(RotateOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "RotateOp(degrees=360.000)");
}

TEST(PaintOpHelper, SaveToString) {
  SaveOp op;
  op.type = static_cast<uint32_t>(SaveOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "SaveOp()");
}

TEST(PaintOpHelper, SaveLayerToString) {
  SkRect bounds = SkRect::MakeXYWH(1, 2, 3, 4);
  SaveLayerOp op(&bounds, nullptr);
  op.type = static_cast<uint32_t>(SaveLayerOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "SaveLayerOp(bounds=[1.000,2.000 3.000x4.000])");
}

TEST(PaintOpHelper, SaveLayerAlphaToString) {
  SkRect bounds = SkRect::MakeXYWH(1, 2, 3, 4);
  SaveLayerAlphaOp op(&bounds, 255, false);
  op.type = static_cast<uint32_t>(SaveLayerAlphaOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str,
            "SaveLayerAlphaOp(bounds=[1.000,2.000 3.000x4.000], alpha=255, "
            "preserve_lcd_text_requests=false)");
}

TEST(PaintOpHelper, ScaleToString) {
  ScaleOp op(12, 13.9f);
  op.type = static_cast<uint32_t>(ScaleOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "ScaleOp(sx=12.000, sy=13.900)");
}

TEST(PaintOpHelper, SetMatrixToString) {
  SetMatrixOp op(SkMatrix::MakeAll(-1, 2, -3, 4, -5, 6, -7, 8, -9));
  op.type = static_cast<uint32_t>(SetMatrixOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str,
            "SetMatrixOp(matrix=[ -1.0000   2.0000  -3.0000][  4.0000  -5.0000 "
            "  6.0000][ -7.0000   8.0000  -9.0000])");
}

TEST(PaintOpHelper, TranslateToString) {
  TranslateOp op(0, 0);
  op.type = static_cast<uint32_t>(TranslateOp::kType);
  std::string str = PaintOpHelper::ToString(&op);
  EXPECT_EQ(str, "TranslateOp(dx=0.000, dy=0.000)");
}

}  // namespace
}  // namespace cc
