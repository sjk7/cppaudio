#include "cppaudio.hpp"
#include <cassert>
#include <iostream>

using namespace std;

void test_sine()
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
    test_sine();
    return 0;
}
