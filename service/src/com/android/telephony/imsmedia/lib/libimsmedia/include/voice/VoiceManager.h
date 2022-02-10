#include <ImsMediaDefine.h>
#include <ImsMediaHal.h>
#include <BaseManager.h>
#include <AudioSession.h>
#include <RtpConfig.h>
#include <unordered_map>
#include <android/content/AttributionSourceState.h>

using namespace std;
using namespace android::telephony::imsmedia;

class VoiceManager : public BaseManager {
public:
    /**
     * @class   RequestHandler
     * @brief   request serialization
     */
    class RequestHandler : public ImsMediaEventHandler {
    public:
        RequestHandler();
        virtual ~RequestHandler();
    protected:
        virtual void processEvent(uint32_t event, uint64_t pParam, uint64_t lParam);
    };

    /**
     * @class   ResponseHandler
     * @brief   ResponseHandler
     *                  has its own thread and sent the response to the client in its own thread
     */
    class ResponseHandler : public ImsMediaEventHandler {
    public:
        ResponseHandler();
        virtual ~ResponseHandler();
    protected:
        virtual void processEvent(uint32_t event, uint64_t pParam, uint64_t lParam);
    };

    static VoiceManager* getInstance();
    static void setAttributeSource(const android::content::AttributionSourceState& client);
    static android::content::AttributionSourceState& getAttributeSource();
    virtual void sendMessage(const int sessionid, const android::Parcel& parcel);

private:
    VoiceManager();
    virtual ~VoiceManager();
    bool openSession(int sessionid, int rtpFd, int rtcpFd, RtpConfig* config);
    ImsMediaHal::RtpError closeSession(int sessionid);
    bool modifySession(int sessionid, RtpConfig* config);
    void addConfig(int sessionid, ImsMediaHal::RtpConfig config);
    bool deleteConfig(int sessionid, RtpConfig* config);
    void confirmConfig(int sessionid, ImsMediaHal::RtpConfig config);
    void startDtmf(int sessionid, char dtmfDigit, int volume, int duration);
    void stopDtmf(int sessionid);
    void sendHeaderExtension(int sessionid, ImsMediaHal::RtpHeaderExtension* data);
    void setMediaQualityThreshold(int sessionid, ImsMediaHal::MediaQualityThreshold threshold);

    static VoiceManager* sManager;
    static android::content::AttributionSourceState mAttributionSource;
    std::unordered_map<int, std::unique_ptr<AudioSession>> mSessions;
    RequestHandler mRequestHandler;
    ResponseHandler mResponseHandler;
};
