#include <stdio.h>
#include <vector>
#include <queue>

#include <AF/AF.h>

static const int MAX_FILES = 16;
static const int MAX_FILE_SIZE = 16384;


struct ReadRequest {
public:

    explicit ReadRequest(
        const AF::Address client = AF::Address(),
        const char *const file_name = 0,
        unsigned char *const buffer = 0,
        const unsigned int buffer_size = 0) 
      : client_(client),
        file_name_(file_name),
        processed_(false),
        buffer_(buffer),
        buffer_size_(buffer_size),
        file_size_(0) {
    }

    void Process() {
        processed_ = true;
        file_size_ = 0;

        FILE *const handle = fopen(file_name_, "rb");
        if (handle != 0) {
            file_size_ = (uint32_t) fread(
                buffer_,
                sizeof(unsigned char),
                buffer_size_,
                handle);

            fclose(handle);
        }
    }

    AF::Address client_;                // Address of the requesting client.
    const char *file_name_;             // Name of the requested file.
    bool processed_;                    // Whether the file has been read.
    unsigned char *buffer_;             // Buffer for file contents.
    unsigned int buffer_size_;          // Size of the buffer.
    unsigned int file_size_;            // Size of the file in bytes.
};


template <class WorkMessage>
class Worker : public AF::Actor {
public:
    Worker(AF::Framework &framework) : AF::Actor(framework) {
        RegisterHandler(this, &Worker::Handler);
    }

private:
    void Handler(const WorkMessage &message, const AF::Address from) {
        WorkMessage result(message);
        result.Process();

        Send(result, from);
    }
};


template <class WorkMessage>
class Dispatcher : public AF::Actor {
public:
    Dispatcher(AF::Framework &framework, const int worker_count) : AF::Actor(framework) {
        for (int i = 0; i < worker_count; ++i) {
            workers_.push_back(new WorkerType(framework));
            free_queue_.push(workers_.back()->GetAddress());
        }

        RegisterHandler(this, &Dispatcher::Handler);
    }

    ~Dispatcher() {
        const int worker_count(static_cast<int>(workers_.size()));
        for (int i = 0; i < worker_count; ++i) {
            delete workers_[i];
        }
    }

private:
    typedef Worker<WorkMessage> WorkerType;

    void Handler(const WorkMessage &message, const AF::Address from) {
        if (message.processed_) {
            Send(message, message.client_);

            free_queue_.push(from);
        } else {
            work_queue_.push(message);
        }

        while (!work_queue_.empty() && !free_queue_.empty()) {
            Send(work_queue_.front(), free_queue_.front());

            free_queue_.pop();
            work_queue_.pop();
        }
    }

    std::vector<WorkerType *> workers_;         // Pointers to the owned workers.
    std::queue<AF::Address> free_queue_;    // Queue of available workers.
    std::queue<WorkMessage> work_queue_;        // Queue of unprocessed work messages.
};


int main(int argc, char *argv[]) {
    AF::Framework::Parameters framework_params;
    framework_params.thread_count_ = MAX_FILES;
    AF::Framework framework(framework_params);

    if (argc < 2) {
        printf("Expected up to 16 file name arguments.\n");
    }

    AF::Receiver receiver;
    AF::Catcher<ReadRequest> result_catcher;
    receiver.RegisterHandler(&result_catcher, &AF::Catcher<ReadRequest>::Push);

    Dispatcher<ReadRequest> dispatcher(framework, MAX_FILES);

    for (int i = 0; i < MAX_FILES && i + 1 < argc; ++i) {
        unsigned char *const buffer = new unsigned char[MAX_FILE_SIZE];
        const ReadRequest message(
            receiver.GetAddress(),
            argv[i + 1],
            buffer,
            MAX_FILE_SIZE);

        framework.Send(message, receiver.GetAddress(), dispatcher.GetAddress());
    }

    for (int i = 1; i < argc; ++i) {
        receiver.Wait();
    }

    ReadRequest result;
    AF::Address from;
    while (!result_catcher.Empty()) {
        result_catcher.Pop(result, from);
        printf("Read %d bytes from file '%s'\n", result.file_size_, result.file_name_);

        delete [] result.buffer_;
    }
}










