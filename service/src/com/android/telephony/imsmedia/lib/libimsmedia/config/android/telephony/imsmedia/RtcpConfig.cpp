#include <RtcpConfig.h>

namespace android {

namespace telephony {

namespace imsmedia {

RtcpConfig::RtcpConfig() {
    transmitPort = 0;
    intervalSec = 0;
    rtcpXrBlockTypes = 0;
}

RtcpConfig::RtcpConfig(RtcpConfig& config) {
    this->canonicalName = config.canonicalName;
    this->transmitPort = config.transmitPort;
    this->intervalSec = config.intervalSec;
    this->rtcpXrBlockTypes = config.rtcpXrBlockTypes;
}

RtcpConfig::~RtcpConfig() {

}

RtcpConfig& RtcpConfig::operator=(const RtcpConfig& config) {
    this->canonicalName = config.canonicalName;
    this->transmitPort = config.transmitPort;
    this->intervalSec = config.intervalSec;
    this->rtcpXrBlockTypes = config.rtcpXrBlockTypes;
    return *this;
}

status_t RtcpConfig::writeToParcel(Parcel* parcel) const {
    (void)parcel;
    return NO_ERROR;
}

status_t RtcpConfig::readFromParcel(const Parcel* in) {
    status_t err;

    err = in->readString16(&canonicalName);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&transmitPort);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&intervalSec);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&rtcpXrBlockTypes);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

String16 RtcpConfig::getCanonicalName() {
    return canonicalName;
}

int32_t RtcpConfig::getTransmitPort() {
    return transmitPort;
}

int32_t RtcpConfig::getIntervalSec() {
    return intervalSec;
}

int32_t RtcpConfig::getRtcpXrBlockTypes() {
    return rtcpXrBlockTypes;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android