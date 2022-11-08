/**
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <ImsMediaImageRotate.h>

class ImsMediaImageRotateTest : public ::testing::Test
{
public:
protected:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(ImsMediaImageRotateTest, Rotate90FlipTest)
{
    const uint32_t img_width = 4, img_height = 4;
    const uint32_t img_buf_size = img_width * img_height * 1.5f;

    // Input image Y buffer
    uint8_t input_img_y[] = {0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33};

    // Input image UV buffer
    uint8_t input_img_uv[] = {80, 90, 81, 91, 82, 92, 83, 93};

    // Expected output Y buffer
    uint8_t exp_img[img_buf_size] = {33, 23, 13, 3, 32, 22, 12, 2, 31, 21, 11, 1, 30, 20, 10, 0, 83,
            93, 81, 91, 82, 92, 80, 90};

    // Output image buffer to be verified
    uint8_t output_img[img_buf_size] = {0};

    ImsMediaImageRotate::YUV420_SP_Rotate90_Flip(
            output_img, input_img_y, input_img_uv, img_width, img_height);

    EXPECT_EQ(memcmp(output_img, exp_img, img_buf_size), 0);
}

TEST_F(ImsMediaImageRotateTest, Rotate90FlipTest_ZeroImageSize)
{
    const uint32_t img_width = 0, img_height = 0;

    // Input image Y buffer
    uint8_t input_img_y[0];

    // Input image UV buffer
    uint8_t input_img_uv[0];

    // Expected output Y buffer
    uint8_t exp_img[0];

    // Output image buffer to be verified
    uint8_t output_img[0];

    ImsMediaImageRotate::YUV420_SP_Rotate90_Flip(
            output_img, input_img_y, input_img_uv, img_width, img_height);

    EXPECT_EQ(memcmp(output_img, exp_img, 0), 0);
}

TEST_F(ImsMediaImageRotateTest, Rotate270Test)
{
    const uint32_t img_width = 4, img_height = 4;
    const uint32_t img_buf_size = img_width * img_height * 1.5f;

    // Input image Y buffer
    uint8_t input_img_y[] = {0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33};

    // Input image UV buffer
    uint8_t input_img_uv[] = {80, 90, 81, 91, 82, 92, 83, 93};

    // Expected output Y buffer
    uint8_t exp_img[img_buf_size] = {3, 13, 23, 33, 2, 12, 22, 32, 1, 11, 21, 31, 0, 10, 20, 30, 81,
            91, 83, 93, 80, 90, 82, 92};

    // Output image buffer to be verified
    uint8_t output_img[img_buf_size] = {0};

    ImsMediaImageRotate::YUV420_SP_Rotate270(
            output_img, input_img_y, input_img_uv, img_width, img_height);

    EXPECT_EQ(memcmp(output_img, exp_img, img_buf_size), 0);
}

TEST_F(ImsMediaImageRotateTest, Rotate270Test_ZeroImageSize)
{
    const uint32_t img_width = 0, img_height = 0;

    // Input image Y buffer
    uint8_t input_img_y[0];

    // Input image UV buffer
    uint8_t input_img_uv[0];

    // Expected output Y buffer
    uint8_t exp_img[0];

    // Output image buffer to be verified
    uint8_t output_img[0];

    ImsMediaImageRotate::YUV420_SP_Rotate270(
            output_img, input_img_y, input_img_uv, img_width, img_height);

    EXPECT_EQ(memcmp(output_img, exp_img, 0), 0);
}

TEST_F(ImsMediaImageRotateTest, Rotate90Planar)
{
    const uint32_t img_width = 4, img_height = 4;
    const uint32_t img_buf_size = img_width * img_height * 1.5f;

    // Input image YUV buffer
    uint8_t input_img[] = {0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33, 80, 81, 82,
            83, 90, 91, 92, 93};

    // Expected output YUV buffer
    uint8_t exp_img[] = {33, 23, 13, 3, 32, 22, 12, 2, 31, 21, 11, 1, 30, 20, 10, 0, 83, 81, 82, 80,
            93, 91, 92, 90};

    // Output image buffer to be verified
    uint8_t output_img[24];

    ImsMediaImageRotate::YUV420_Planar_Rotate90_Flip(output_img, input_img, img_width, img_height);

    EXPECT_EQ(memcmp(output_img, exp_img, img_buf_size), 0);
}

TEST_F(ImsMediaImageRotateTest, Rotate90PlanarTest_ZeroImageSize)
{
    const uint32_t img_width = 0, img_height = 0;

    // Input image YUV buffer
    uint8_t input_img[0];

    // Expected output YUV buffer
    uint8_t exp_img[0];

    // Output image buffer to be verified
    uint8_t output_img[0];

    ImsMediaImageRotate::YUV420_Planar_Rotate90_Flip(output_img, input_img, img_width, img_height);

    EXPECT_EQ(memcmp(output_img, exp_img, 0), 0);
}