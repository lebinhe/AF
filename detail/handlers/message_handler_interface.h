#ifndef AF_DETAIL_HANDLERS_MESSAGEHANDLER_INTERFACE_H
#define AF_DETAIL_HANDLERS_MESSAGEHANDLER_INTERFACE_H


#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"

#include "AF/detail/messages/message_interface.h"


namespace AF
{

class Actor;

namespace Detail
{

class MessageHandlerInterface : public List<MessageHandlerInterface>::Node {
public:
    AF_FORCEINLINE MessageHandlerInterface() : marked_(false), predict_send_count_(0) {
    }

    inline virtual ~MessageHandlerInterface() {
    }

    // Marks the handler (eg. for deletion).
    inline void Mark();

    inline bool IsMarked() const;

    // Reports the number of messages sent by an invocation of the handler.
    inline void ReportSendCount(const uint32_t count);

    // Gets a prediction of the number of messages that would be sent by the handler when invoked.
    inline uint32_t GetPredictedSendCount() const;

    // Returns the unique name of the message type handled by this handler.
    virtual const char *GetMessageTypeName() const = 0;

    virtual bool Handle(Actor *const actor, const MessageInterface *const message) = 0;

private:
    MessageHandlerInterface(const MessageHandlerInterface &other);
    MessageHandlerInterface &operator=(const MessageHandlerInterface &other);

    bool marked_;                   // Flag used to mark the handler for deletion.
    uint32_t predict_send_count_;   // Number of messages that are predicted to be sent by the handler.
};


AF_FORCEINLINE void MessageHandlerInterface::Mark() {
    marked_ = true;
}

AF_FORCEINLINE bool MessageHandlerInterface::IsMarked() const {
    return marked_;
}

AF_FORCEINLINE void MessageHandlerInterface::ReportSendCount(const uint32_t count) {
    // For now we assume that the message send count will be the same as last time.
    if (predict_send_count_ != count) {
        predict_send_count_ = count;
    }
}

AF_FORCEINLINE uint32_t MessageHandlerInterface::GetPredictedSendCount() const {
    return predict_send_count_;
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_MESSAGEHANDLER_INTERFACE_H
