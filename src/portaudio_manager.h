#ifndef PORTAUDIO_MANAGER_H
#define PORTAUDIO_MANAGER_H

#include <portaudio.h>
#include <mutex>

class PortAudioManager
{
public:
    static void initialize()
    {
        std::lock_guard<std::mutex> lock(mutex());
        if (refCount()++ == 0) {
            PaError err = Pa_Initialize();
            if (err != paNoError) {
                refCount()--;
            }
        }
    }

    static void terminate()
    {
        std::lock_guard<std::mutex> lock(mutex());
        if (refCount() > 0 && --refCount() == 0) {
            Pa_Terminate();
        }
    }

private:
    static std::mutex &mutex()
    {
        static std::mutex m;
        return m;
    }

    static int &refCount()
    {
        static int count = 0;
        return count;
    }
};

#endif // PORTAUDIO_MANAGER_H
