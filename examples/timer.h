#ifndef AF_TIMER_H
#define AF_TIMER_H

#include <AF/AF.h>

#include <sys/time.h>


class Timer {
public:

    Timer() : supported_(true) {
    }

    void Start() {
        gettimeofday(&t1, NULL);
    }

    void Stop() {
        gettimeofday(&t2, NULL);
    }

    bool Supported() const {
        return supported_;
    }

    float Seconds() const {
        return (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 1e-6f;
    }

private:

    Timer(const Timer &other);
    Timer &operator=(const Timer &other);

    bool supported_;

    timeval t1, t2;

};

#endif // AF_BENCHMARKS_COMMON_TIMER_H

