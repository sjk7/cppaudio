#pragma once
#include "./portaudio/include/portaudio.h"
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <objbase.h> // CoInitialize: missing calls in some versions of PortAudio?
struct ComInit
{
    HRESULT m_hr;
    ComInit() { m_hr = CoInitialize(NULL); }
    ~ComInit()
    {
        if (SUCCEEDED(m_hr)) CoUninitialize();
    }
};
#else
#define ComInit
#endif

namespace cppaudio
{
namespace detail
{
template <typename T> struct NoCopy
{
    NoCopy() = default;
    NoCopy &operator=(const NoCopy &) = delete;
    NoCopy(const NoCopy &) = delete;
};
struct PaStatics : NoCopy<PaStatics>, ComInit
{
    PaStatics()
    {
        Pa_Initialize();
        ctr++;
    }
    virtual ~PaStatics()
    {
        if (ctr-- == 0) Pa_Terminate();
    }
    static auto instances() noexcept { return ctr; }
    static std::string_view build_info() noexcept
    {
        return Pa_GetVersionInfo()->versionText;
    }
    static auto DeviceCount() noexcept { return Pa_GetDeviceCount(); }
    static auto ApiCount() noexcept { return Pa_GetHostApiCount(); }

  private:
    static inline unsigned int ctr = 0;
};
} // namespace detail

class SystemDevice
{
    PaDeviceInfo info;
    int m_global_device_index = 0;

  public:
    SystemDevice(const PaDeviceInfo *info, int global_device_index) 
        : info(*info), m_global_device_index(global_device_index) {}

    const std::string_view name() const noexcept { return info.name; }
    const PaDeviceInfo &Info() const noexcept { return info; }
    int GlobalDeviceIndex() const noexcept { return m_global_device_index; };
};
using SystemDeviceList = std::vector<SystemDevice>;
class Device
{
    SystemDevice m_sysDevice;

  public:
    Device(const SystemDevice &sysdevice) : m_sysDevice(sysdevice) {}
};

class HostApi;
class DeviceEnum
{
    friend class HostApi;
    detail::PaStatics *m_pa = nullptr;
    mutable SystemDeviceList m_deviceList;
    const PaHostApiInfo *m_api;

    DeviceEnum(detail::PaStatics &pa, const PaHostApiInfo *api)
        : m_pa(&pa), m_api(api)
    {
    }
    const SystemDeviceList &Devices() const
    {
        if (m_deviceList.empty())
        {
            DeviceEnum *pthis = const_cast<DeviceEnum *>(this);
            pthis->populate();
        }
        return m_deviceList;
    }

    void populate()
    {
        m_deviceList.clear();
        const auto myapiIndex = Pa_HostApiTypeIdToHostApiIndex(m_api->type);
        for (auto i = 0; i < m_pa->DeviceCount(); ++i)
        {
            auto info = Pa_GetDeviceInfo(i);
            if (info->hostApi == myapiIndex)
            {
                m_deviceList.emplace_back(SystemDevice(info, i));
            }
        }
    }

  public:
    auto Count() const noexcept { return m_pa->DeviceCount(); }
};

enum class HostIds
{
    InDevelopment = 0,
    DirectSound = 1,
    MME = 2,
    ASIO = 3,
    SoundManager = 4,
    CoreAudio = 5,
    OSS = 7,
    ALSA = 8,
    AL = 9,
    BeOS = 10,
    WDMKS = 11,
    JACK = 12,
    WASAPI = 13,
    AudioScienceHPI = 14
};

static const std::vector<std::string> HostIdNames = {
    "InDevelopment", "DirectSound", "MME",         "ASIO", "SoundManager",
    "CoreAudio",     "OSS",         "ALSA",        "AL",   "BeOS",
    "WDKMS",         "JACK",        "AudioScience"};

[[maybe_unused]] static inline const std::string_view
HostIdToString(const HostIds hostId)
{
    int i = (int)hostId;
    if (i >= HostIdNames.size())
    {
        return "ERROR: BAD HOST ID";
    }
    else
    {
        return HostIdNames.at(i);
    }
}

class HostApi
{
    friend class audio;
    DeviceEnum m_enum;
    HostIds m_hostid;
    PaHostApiInfo info;

  public:
    HostApi(const PaHostApiInfo *info, detail::PaStatics &pa, HostIds hostId)
        : m_enum(pa, info), m_hostid(hostId), info(*info)
    {
    }
    HostApi(const HostApi &rhs)
        : m_enum(rhs.m_enum), m_hostid(rhs.m_hostid), info(rhs.info)
    {
    }

    HostApi &operator=(const HostApi &rhs)
    {
        m_enum = rhs.m_enum;
        m_hostid = rhs.m_hostid;
        info = rhs.info;
        return *this;
    }

    using HostApiList = std::vector<HostApi>;

    std::string_view name() const noexcept { return info.name; }
    const auto &Devices() const noexcept { return m_enum.Devices(); }
    HostIds HostId() const noexcept { return m_hostid; }
    const PaHostApiInfo &Info() const noexcept { return info; }
    const std::optional<SystemDevice> DefaultOutputDevice() const {

        const auto glob_idx = info.defaultOutputDevice;
        for (const auto &sd : m_enum.Devices())
        {
            if (sd.GlobalDeviceIndex() == glob_idx)
            {
                return sd;
            }
        }
        // wasn't found, accept the first?
        if (Devices().size() > 0)
            return Devices().at(0);
        else
            return std::optional<SystemDevice>();
    }
};
using HostApiList = std::vector<HostApi>;
class ApiEnumerator
{
    friend class audio;
    HostApiList m_apis;
    detail::PaStatics &m_pa;
    HostApi DefaultHostApi() const noexcept
    {
        auto index = Pa_GetDefaultHostApi();
        auto info = Pa_GetHostApiInfo(index);
        return HostApi(info, m_pa, (HostIds)info->type);
    }
    ApiEnumerator(detail::PaStatics &pa) : m_pa(pa) { do_enum(); }
    void do_enum()
    {
        m_apis.clear();
        for (auto i = 0; i < m_pa.ApiCount(); ++i)
        {
            auto info = Pa_GetHostApiInfo(i);
            HostIds mytype = (HostIds)info->type;
            HostApi a(info, m_pa, mytype);
            m_apis.push_back(a);
        }
    }
};
using HostApiResult = std::optional<HostApi>;
class audio
{
    static inline detail::PaStatics m_pa;
    ApiEnumerator m_enum;
    HostApiResult m_api_current = m_enum.DefaultHostApi();

  public:
    audio() : m_enum(m_pa)
    {
        m_api_current = m_enum.DefaultHostApi();
        if (!m_api_current.has_value())
        {
            throw std::runtime_error(
                "audio object cannot find a valid backend api");
        }
    }
    audio(const HostIds hostId) : m_enum(m_pa)
    {
        m_api_current = FindHostApi(hostId);
        if (!m_api_current.has_value())
        {
            throw std::runtime_error(
                "audio object cannot find a valid backend api");
        }
    }
    audio(const HostApi api) : m_enum(m_pa)
    {
        m_api_current = api;
        if (!m_api_current.has_value())
        {
            throw std::runtime_error(
                "audio object cannot find a valid backend api");
        }
    }
    static auto build_info() noexcept
    {
        return detail::PaStatics::build_info();
    }
    const HostApiList hostApis() const noexcept { return m_enum.m_apis; }
    const HostApiResult &CurrentApi() const noexcept { return m_api_current; }
    const HostApi DefaultApi() const { return m_enum.DefaultHostApi(); }

    // returns a copy of the HostApi, if found
    HostApiResult FindHostApi(HostIds hostId)
    {
        for (const auto &a : hostApis())
        {
            if (a.HostId() == hostId)
            {
                return HostApi(a);
            }
        }
        return HostApiResult();
    }
    HostApiResult FindHostApi(std::string_view name)
    {
        for (const auto &a : hostApis())
        {
            if (a.name() == name)
            {
                return HostApi(a);
            }
        }
        return HostApiResult();
    }
};

} // namespace cppaudio
