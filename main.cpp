// This is an independent project of an individual developer. Dear PVS-Studio,
// please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// http://www.viva64.com

#include "cppaudio.hpp"
#include <cassert>
#include <iostream>

using namespace std;

namespace SAMPLETYPES
{
using INT16 = short;
using FLOAT32 = float;
using DOUBLE32 = DOUBLE;
} // namespace SAMPLETYPES

template <typename SAMPLETYPE, typename CB> struct Stream
{
    Stream(CB &&cb) : m_cb(std::forward<CB>(cb)){};
    Stream(cppaudio::Device &, CB &&cb) : Stream(std::forward<CB>(cb)) {}
    CB m_cb;
    template <typename T> void call(const T &t) { m_cb(t); }
};

void play_tone()
{

    cppaudio::audio a;
    auto myDevice = a.DefaultOutputDeviceInstance();
    auto mystream = Stream(myDevice, [](auto &params) {
        cout << params << endl;
        return 0;
    });
    mystream.call("Hello wanka!"s);
    mystream.call(42);
    cout << endl;
}

void test_output_device_prepare()
{
#ifdef _WIN32
    const cppaudio::HostIds THE_HOST_ID = cppaudio::HostIds::WASAPI;
#else
    cppaudio::audio x;
    const cppaudio::HostIds THE_HOST_ID = x.DefaultApi().HostId();
#endif

    cppaudio::audio a(THE_HOST_ID);
    auto &hostapi = a.CurrentApi();

    auto sysDevice = *hostapi->DefaultOutputDevice();
    auto mydevice = cppaudio::Device(sysDevice, cppaudio::Direction::output);
    cout << mydevice.name() << endl;
    assert(mydevice.name() == sysDevice.name());
    assert(mydevice.IsOutput());
    assert(mydevice.hasOutputParams());
    assert(!mydevice.hasInputParams());
    assert(mydevice.sampleFormat() == cppaudio::SampleFormats::Float32);
}

void test_api(cppaudio::audio &audio)
{
#ifdef _WIN32

    const cppaudio::HostIds THE_HOST_ID = cppaudio::HostIds::WASAPI;
#else
    const cppaudio::HostIds THE_HOST_ID = cppaudio::HostIds::CoreAudio;
#endif
    auto a = audio.FindHostApi(THE_HOST_ID);
    auto b = audio.FindHostApi(a->name());
    assert(a->HostId() == b->HostId());
    auto c = audio.FindHostApi("meh");
    assert(!c.has_value());
    auto d = audio.DefaultApi();
    cout << "\nDefault API on this system is: " << d.name() << endl;

    if (!a.has_value())
    {
        cerr << "Unable to find host with HostId: "
             << cppaudio::HostIdToString(cppaudio::HostIds::WASAPI) << endl;
        assert("HostId not found" == nullptr);
    }

    cppaudio::audio myaudio(*a);
    assert(myaudio.CurrentApi()->HostId() == THE_HOST_ID);
}

int main()
{
    play_tone();
    exit(0);
    cppaudio::audio audio;
    cout << "Using backend: " << cppaudio::audio::build_info() << endl;
    cout << "Host Apis available: " << audio.hostApis().size() << endl;
    for (const auto &a : audio.hostApis())
    {
        cout << a.name() << endl;
    }
    cout << endl;

    for (const auto &a : audio.hostApis())
    {
        cout << "Devices, for api: " << a.name() << ":" << endl;
        for (const auto &d : a.Devices())
        {
            cout << "\t" << d.name() << endl;
        }
    }

    test_api(audio);
    test_output_device_prepare();
    return 0;
}
