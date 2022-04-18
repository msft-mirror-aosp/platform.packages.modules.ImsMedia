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

#ifndef BASE_NODE_ID_H
#define BASE_NODE_ID_H

enum BaseNodeID {
    //for socket
    NODEID_SOCKETWRITER,
    NODEID_SOCKETREADER,
    //NODEID_PCAPPARSER,
    //for rtp
    NODEID_RTPENCODER,
    NODEID_RTPDECODER,
    //for rtcp
    NODEID_RTCPENCODER,
    NODEID_RTCPDECODER,
    //for audio
    NODEID_AUDIOSOURCE,
    NODEID_AUDIOPLAYER,
    NODEID_DTMFENCODER,
    NODEID_DTMFSENDER,
    NODEID_RTPPAYLOAD_ENCODER_AUDIO,
    NODEID_RTPPAYLOAD_DECODER_AUDIO,
    //for video
    // NODEID_VIDEOSOURCE,
    // NODEID_VIDEORENDERER,
    // NODEID_VIDEODECODER,
    // NODEID_RTPPAYLOAD_ENCODER_VIDEO,
    // NODEID_RTPPAYLOAD_DECODER_VIDEO,
    //for text
    // NODEID_TEXTSOURCE,
    // NODEID_TEXTRENDERER,
    // NODEID_RTPPAYLOAD_ENCODER_TEXT,
    // NODEID_RTPPAYLOAD_DECODER_TEXT,
    NODEID_MAX,
};

#endif