#include "cppaudio.hpp"
#include <iostream>
#include <cassert>


using namespace std;

void test_sine() {
    #ifdef _WIN32
    const cppaudio::HostIds THE_HOST_ID = cppaudio::HostIds::WASAPI;
    #else
    cppaudio::audio a;
    const cppaudio::HostIds THE_HOST_ID = a.DefaultApi().HostId();
    #endif


    cppaudio::audio a(THE_HOST_ID);
    auto& hostapi = a.CurrentApi().value();
    
    auto& sysDevice = hostapi.DefaultOutputDevice();
    assert(sysDevice.has_value());


}

void test_api(cppaudio::audio& audio) {
    
    const cppaudio::HostIds THE_HOST_ID = cppaudio::HostIds::WASAPI;
    auto a = audio.FindHostApi(THE_HOST_ID);
    auto b = audio.FindHostApi(a.value().name());
    assert(a.value().HostId() == b.value().HostId());
    auto c = audio.FindHostApi("meh");
    assert(!c.has_value());
    auto d = audio.DefaultApi();
    cout << "\nDefault API on this system is: " << d.name() << endl;


    if (!a.has_value())
    {
        cerr << "Unable to find host with HostId: " << cppaudio::HostIdToString(cppaudio::HostIds::WASAPI)
             << endl;
        assert("HostId not found" == nullptr);
    }

    cppaudio::audio myaudio(a.value());
    assert(myaudio.CurrentApi().value().HostId() == THE_HOST_ID);

}

int main()
{

    cppaudio::audio audio;
    cout << "Using backend: " << cppaudio::audio::build_info() << endl;
    cout << "Host Apis available: " << audio.hostApis().size() << endl;
    for (const auto &a : audio.hostApis()) {
        cout << a.name() << endl;
    }
    cout << endl;

    for (const auto &a : audio.hostApis()) {
        cout << "Devices, for api: " << a.name() << ":" << endl;
        for (const auto &d : a.Devices()) {
            cout << "\t" << d.name() << endl;
        }
    }
    
    test_api(audio);
    test_sine();
    return 0;
}
