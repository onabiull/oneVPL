/*############################################################################
  # Copyright (C) 2021 Intel Corporation
  #
  # SPDX-License-Identifier: MIT
  ############################################################################*/

#include "vpl_implementation_loader.h"
#include "sample_defs.h"
#include "sample_utils.h"
#include "vpl/mfxdispatcher.h"

#if defined(LINUX32) || defined(LINUX64)
    #include <link.h>
    #include <string.h>
#endif

#include <map>
#include <regex>

static mfxI32 GetAdapterNumber(const mfxChar* cDeviceID) {
    std::string strDevID(cDeviceID);
    size_t idx        = strDevID.rfind('/');
    mfxI32 adapterIdx = -1;

    if (idx != std::string::npos && (idx + 1) < strDevID.size())
        adapterIdx = std::stoi(strDevID.substr(idx + 1));

    return adapterIdx;
}

const std::map<mfxAccelerationMode, const msdk_tstring> mfxAccelerationModeNames = {
    { MFX_ACCEL_MODE_NA, MSDK_STRING("MFX_ACCEL_MODE_NA") },
    { MFX_ACCEL_MODE_VIA_D3D9, MSDK_STRING("MFX_ACCEL_MODE_VIA_D3D9") },
    { MFX_ACCEL_MODE_VIA_D3D11, MSDK_STRING("MFX_ACCEL_MODE_VIA_D3D11") },
    { MFX_ACCEL_MODE_VIA_VAAPI, MSDK_STRING("MFX_ACCEL_MODE_VIA_VAAPI") },
    { MFX_ACCEL_MODE_VIA_VAAPI_DRM_RENDER_NODE,
      MSDK_STRING("MFX_ACCEL_MODE_VIA_VAAPI_DRM_RENDER_NODE") },
    { MFX_ACCEL_MODE_VIA_VAAPI_DRM_MODESET, MSDK_STRING("MFX_ACCEL_MODE_VIA_VAAPI_DRM_MODESET") },
    { MFX_ACCEL_MODE_VIA_VAAPI_GLX, MSDK_STRING("MFX_ACCEL_MODE_VIA_VAAPI_GLX") },
    { MFX_ACCEL_MODE_VIA_VAAPI_X11, MSDK_STRING("MFX_ACCEL_MODE_VIA_VAAPI_X11") },
    { MFX_ACCEL_MODE_VIA_VAAPI_WAYLAND, MSDK_STRING("MFX_ACCEL_MODE_VIA_VAAPI_WAYLAND") },
    { MFX_ACCEL_MODE_VIA_HDDLUNITE, MSDK_STRING("MFX_ACCEL_MODE_VIA_HDDLUNITE") }
};

VPLImplementationLoader::VPLImplementationLoader() {
    m_loader.reset(MFXLoad(), MFXUnload);
    m_Loader = m_loader.get();

    m_ImplIndex   = 0;
    m_Impl        = MFX_IMPL_TYPE_HARDWARE;
    m_MinVersion  = mfxVersion{ 0 /*minor*/, 1 /*major*/ };
    m_adapterType = mfxMediaAdapterType::MFX_MEDIA_UNKNOWN;
    m_dGfxIdx     = -1;
    m_adapterNum  = -1;
}

VPLImplementationLoader::~VPLImplementationLoader() {}

mfxStatus VPLImplementationLoader::CreateConfig(char const* data, const char* propertyName) {
    mfxConfig cfg = MFXCreateConfig(m_Loader);
    mfxVariant variant;
    variant.Type     = MFX_VARIANT_TYPE_PTR;
    variant.Data.Ptr = mfxHDL(data);
    mfxStatus sts    = MFXSetConfigFilterProperty(cfg, (mfxU8*)propertyName, variant);
    MSDK_CHECK_STATUS(sts, "MFXSetConfigFilterProperty failed");

    return sts;
}

mfxStatus VPLImplementationLoader::CreateConfig(mfxU16 data, const char* propertyName) {
    mfxConfig cfg = MFXCreateConfig(m_Loader);
    mfxVariant variant;
    variant.Type     = MFX_VARIANT_TYPE_U16;
    variant.Data.U32 = data;
    mfxStatus sts    = MFXSetConfigFilterProperty(cfg, (mfxU8*)propertyName, variant);
    MSDK_CHECK_STATUS(sts, "MFXSetConfigFilterProperty failed");

    return sts;
}

mfxStatus VPLImplementationLoader::CreateConfig(mfxU32 data, const char* propertyName) {
    mfxConfig cfg = MFXCreateConfig(m_Loader);
    mfxVariant variant;
    variant.Type     = MFX_VARIANT_TYPE_U32;
    variant.Data.U32 = data;
    mfxStatus sts    = MFXSetConfigFilterProperty(cfg, (mfxU8*)propertyName, variant);
    MSDK_CHECK_STATUS(sts, "MFXSetConfigFilterProperty failed");

    return sts;
}

mfxStatus VPLImplementationLoader::ConfigureImplementation(mfxIMPL impl) {
    mfxConfig cfgImpl = MFXCreateConfig(m_Loader);

    std::vector<mfxU32> hwImpls = { MFX_IMPL_HARDWARE,  MFX_IMPL_HARDWARE_ANY, MFX_IMPL_HARDWARE2,
                                    MFX_IMPL_HARDWARE3, MFX_IMPL_HARDWARE4,    MFX_IMPL_VIA_D3D9,
                                    MFX_IMPL_VIA_D3D11 };

    std::vector<mfxU32>::iterator hwImplsIt =
        std::find_if(hwImpls.begin(), hwImpls.end(), [impl](const mfxU32& val) {
            return (val == MFX_IMPL_VIA_MASK(impl) || val == MFX_IMPL_BASETYPE(impl));
        });

    if (MFX_IMPL_BASETYPE(impl) == MFX_IMPL_SOFTWARE) {
        m_Impl = MFX_IMPL_TYPE_SOFTWARE;
    }
    else if (hwImplsIt != hwImpls.end()) {
        m_Impl = MFX_IMPL_TYPE_HARDWARE;
    }
    else {
        return MFX_ERR_UNSUPPORTED;
    }

    mfxVariant ImplVariant;
    ImplVariant.Type     = MFX_VARIANT_TYPE_U32;
    ImplVariant.Data.U32 = m_Impl;
    mfxStatus sts =
        MFXSetConfigFilterProperty(cfgImpl, (mfxU8*)"mfxImplDescription.Impl", ImplVariant);
    MSDK_CHECK_STATUS(sts, "MFXSetConfigFilterProperty failed");
    msdk_printf(
        MSDK_STRING("CONFIGURE LOADER: required implemetation: %s \n"),
        ImplVariant.Data.U32 == MFX_IMPL_TYPE_HARDWARE ? MSDK_STRING("hw") : MSDK_STRING("sw"));
    return sts;
}

mfxStatus VPLImplementationLoader::ConfigureAccelerationMode(mfxAccelerationMode accelerationMode,
                                                             mfxIMPL impl) {
    mfxStatus sts = MFX_ERR_NONE;
    bool isHW     = MFX_IMPL_BASETYPE(impl) != MFX_IMPL_SOFTWARE;

    // configure accelerationMode, except when required implementation is MFX_IMPL_TYPE_HARDWARE, but m_accelerationMode not set
    if (accelerationMode != MFX_ACCEL_MODE_NA || !isHW) {
        sts = CreateConfig((mfxU32)accelerationMode, "mfxImplDescription.AccelerationMode");
        msdk_printf(
            MSDK_STRING("CONFIGURE LOADER: required implemetation mfxAccelerationMode: %s \n"),
            mfxAccelerationModeNames.at(accelerationMode).c_str());
    }

    return sts;
}

mfxStatus VPLImplementationLoader::ConfigureVersion(mfxVersion const version) {
    mfxStatus sts = MFX_ERR_NONE;

    sts = CreateConfig(version.Version, "mfxImplDescription.ApiVersion.Version");
    sts = CreateConfig(version.Major, "mfxImplDescription.ApiVersion.Major");
    sts = CreateConfig(version.Minor, "mfxImplDescription.ApiVersion.Minor");
    msdk_printf(MSDK_STRING("CONFIGURE LOADER: required version: %d.%d.%d\n"),
                version.Major,
                version.Minor,
                version.Version);

    return sts;
}

void VPLImplementationLoader::SetAdapterType(mfxU16 adapterType) {
    if (m_adapterNum != -1) {
        msdk_printf(MSDK_STRING(
            "CONFIGURE LOADER: required adapter type may conflict with adapter number, adapter type will be ignored \n"));
    }
    else if (adapterType != mfxMediaAdapterType::MFX_MEDIA_UNKNOWN) {
        m_adapterType = adapterType;

        msdk_stringstream ss;
        ss << MSDK_STRING("CONFIGURE LOADER: required adapter type: ")
           << (m_adapterType == mfxMediaAdapterType::MFX_MEDIA_INTEGRATED
                   ? MSDK_STRING("integrated")
                   : MSDK_STRING("discrete"));
        ss << std::endl;

        msdk_printf(MSDK_STRING("%s"), ss.str().c_str());
    }
}

void VPLImplementationLoader::SetDiscreteAdapterIndex(mfxI32 dGfxIdx) {
    if (m_adapterNum != -1) {
        msdk_printf(MSDK_STRING(
            "CONFIGURE LOADER: required adapter type may conflict with adapter number, adapter type will be ignored \n"));
    }
    else if (dGfxIdx >= 0) {
        m_adapterType = mfxMediaAdapterType::MFX_MEDIA_DISCRETE;
        m_dGfxIdx     = dGfxIdx;
        msdk_printf(MSDK_STRING("CONFIGURE LOADER: required discrete adapter index %d \n"),
                    m_dGfxIdx);
    }
}

void VPLImplementationLoader::SetAdapterNum(mfxI32 adapterNum) {
    if (adapterNum >= 0) {
        if (m_adapterType != mfxMediaAdapterType::MFX_MEDIA_UNKNOWN) {
            m_adapterType = mfxMediaAdapterType::MFX_MEDIA_UNKNOWN;
            m_dGfxIdx     = -1;
            msdk_printf(MSDK_STRING(
                "CONFIGURE LOADER: required adapter type may conflict with adapter number, adapter type will be ignored \n"));
        }
        m_adapterNum = adapterNum;
        msdk_printf(MSDK_STRING("CONFIGURE LOADER: required adapter number: %d \n"), m_adapterNum);
    }
}

mfxStatus VPLImplementationLoader::EnumImplementations() {
    mfxImplDescription* idesc = nullptr;
    mfxStatus sts             = MFX_ERR_NONE;

    std::vector<std::pair<mfxU32, mfxImplDescription*>> unique_devices;

    m_ImplIndex = (mfxU32)-1;
    for (int impl = 0; sts == MFX_ERR_NONE; impl++) {
        sts =
            MFXEnumImplementations(m_Loader, impl, MFX_IMPLCAPS_IMPLDESCSTRUCTURE, (mfxHDL*)&idesc);
        if (!idesc) {
            sts = MFX_ERR_NONE;
            break;
        }
        else if (idesc->ApiVersion < m_MinVersion) {
            continue;
        }

        auto it = std::find_if(unique_devices.begin(),
                               unique_devices.end(),
                               [idesc](const std::pair<mfxU32, mfxImplDescription*> val) {
                                   return (GetAdapterNumber(idesc->Dev.DeviceID) ==
                                           GetAdapterNumber(val.second->Dev.DeviceID));
                               });

        if (it == unique_devices.end()) {
            unique_devices.push_back(std::make_pair(impl, idesc));
            if (m_adapterNum == -1 && m_adapterType == mfxMediaAdapterType::MFX_MEDIA_UNKNOWN &&
                idesc->Dev.MediaAdapterType == mfxMediaAdapterType::MFX_MEDIA_INTEGRATED) {
                m_adapterType = mfxMediaAdapterType::MFX_MEDIA_INTEGRATED;
                break;
            }
        }
    }

    if (unique_devices.empty()) {
        msdk_printf(MSDK_STRING("Library was not found with required version \n"));
        return MFX_ERR_NOT_FOUND;
    }

    std::sort(unique_devices.begin(),
              unique_devices.end(),
              [](const std::pair<mfxU32, mfxImplDescription*>& left,
                 const std::pair<mfxU32, mfxImplDescription*>& right) {
                  return GetAdapterNumber(left.second->Dev.DeviceID) <
                         GetAdapterNumber(right.second->Dev.DeviceID);
              });

    mfxI32 dGfxIdx = -1;
    for (const auto& it : unique_devices) {
        if (it.second->Dev.MediaAdapterType == mfxMediaAdapterType::MFX_MEDIA_DISCRETE) {
            dGfxIdx++;
        }

        if ((m_adapterType == mfxMediaAdapterType::MFX_MEDIA_UNKNOWN ||
             m_adapterType == it.second->Dev.MediaAdapterType) &&
            (m_adapterNum == -1 || m_adapterNum == GetAdapterNumber(it.second->Dev.DeviceID)) &&
            (m_dGfxIdx == -1 || m_dGfxIdx == dGfxIdx)) {
            m_idesc.reset(it.second, [this](mfxImplDescription* d) {
                MFXDispReleaseImplDescription(m_Loader, d);
            });

            m_ImplIndex = it.first;
            break;
        }
    }

    if (m_ImplIndex == (mfxU32)-1) {
        msdk_printf(MSDK_STRING("Library was not found with required adapter type/num \n"));
        sts = MFX_ERR_NOT_FOUND;
    }

    return sts;
}

mfxStatus VPLImplementationLoader::ConfigureAndEnumImplementations(
    mfxIMPL impl,
    mfxAccelerationMode accelerationMode) {
    mfxStatus sts = ConfigureImplementation(impl);
    MSDK_CHECK_STATUS(sts, "ConfigureImplementation failed");
    sts = ConfigureAccelerationMode(accelerationMode, impl);
    MSDK_CHECK_STATUS(sts, "ConfigureAccelerationMode failed");

#if !defined(_WIN32)
    sts = EnumImplementations();
    MSDK_CHECK_STATUS(sts, "EnumImplementations failed");
#else
    if (m_Impl != MFX_IMPL_HARDWARE || m_adapterType != mfxMediaAdapterType::MFX_MEDIA_UNKNOWN) {
        sts = EnumImplementations();
        MSDK_CHECK_STATUS(sts, "EnumImplementations failed");
    }
    else {
        //Low-latency mode
        msdk_printf(MSDK_STRING("CONFIGURE LOADER: Use dispatcher's low-latency mode\n"));

        sts = CreateConfig("mfx-gen", "mfxImplDescription.ImplName");
        MSDK_CHECK_STATUS(sts, "Failed to configure mfxImplDescription.ImplName");

        sts = CreateConfig(mfxU32(0x8086), "mfxImplDescription.VendorID");
        MSDK_CHECK_STATUS(sts, "Failed to configure mfxImplDescription.VendorID");

        if (m_adapterNum == -1)
            m_adapterNum = 0;

        sts = CreateConfig(mfxU32(m_adapterNum), "DXGIAdapterIndex");
        MSDK_CHECK_STATUS(sts, "Failed to configure DXGIAdapterIndex");

        //only one impl. is reported
        m_ImplIndex = 0;
        //no 'mfxImplDescription' is available
        m_idesc.reset(new mfxImplDescription());
        m_idesc->ApiVersion       = m_MinVersion;
        m_idesc->Impl             = m_Impl;
        m_idesc->AccelerationMode = accelerationMode;
        snprintf(m_idesc->ImplName, sizeof(m_idesc->ImplName), "mfx-gen");
        m_idesc->VendorID = 0x8086;
        snprintf(m_idesc->Dev.DeviceID,
                 sizeof(m_idesc->Dev.DeviceID),
                 "%x/%d",
                 0 /* no ID */,
                 m_adapterNum);
    }
#endif

    return sts;
}

mfxLoader VPLImplementationLoader::GetLoader() const {
    return m_Loader;
}

mfxU32 VPLImplementationLoader::GetImplIndex() const {
    return m_ImplIndex;
}

mfxVersion VPLImplementationLoader::GetVersion() const {
    return m_idesc ? m_idesc->ApiVersion : mfxVersion({ { 0, 0 } });
}

std::string VPLImplementationLoader::GetImplName() const {
    if (m_idesc) {
        return std::string(m_idesc->ImplName);
    }
    else {
        return "";
    }
}

std::pair<mfxI16, mfxI32> VPLImplementationLoader::GetDeviceIDAndAdapter() const {
    auto result = std::make_pair(-1, -1);
    if (!m_idesc)
        return result;

    std::string deviceAdapterInfo(m_idesc->Dev.DeviceID);
    std::regex pattern("([0-9a-fA-F]+)(?:/([0-9]|[1-9][0-9]+))?");
    std::smatch match;
    if (!std::regex_match(deviceAdapterInfo, match, pattern))
        return result;

    try {
        result.first = std::stoi(match[1].str(), 0, 16);
        if (match[2].matched)
            result.second = std::stoi(match[2].str());
    }
    catch (std::exception const&) {
        return result;
    }

    return result;
}

mfxU16 VPLImplementationLoader::GetAdapterType() const {
    return m_idesc ? m_idesc->Dev.MediaAdapterType : mfxMediaAdapterType::MFX_MEDIA_UNKNOWN;
}

void VPLImplementationLoader::SetMinVersion(mfxVersion const& version) {
    m_MinVersion = version;
}

mfxStatus MainVideoSession::CreateSession(VPLImplementationLoader* Loader) {
    mfxStatus sts      = MFXCreateSession(Loader->GetLoader(), Loader->GetImplIndex(), &m_session);
    mfxVersion version = Loader->GetVersion();
    msdk_printf(MSDK_STRING("Loaded Library Version: %d.%d \n"), version.Major, version.Minor);
    std::string implName = Loader->GetImplName();
    msdk_tstring strImplName;
    std::copy(std::begin(implName), std::end(implName), back_inserter(strImplName));
    msdk_printf(MSDK_STRING("Loaded Library ImplName: %s \n"), strImplName.c_str());

#if (defined(LINUX32) || defined(LINUX64))
    msdk_printf(MSDK_STRING("Used implementation number: %d \n"), Loader->GetImplIndex());
    msdk_printf(MSDK_STRING("Loaded modules:\n"));
    int numLoad = 0;
    dl_iterate_phdr(PrintLibMFXPath, &numLoad);
#else
    #if !defined(MFX_DISPATCHER_LOG)
    PrintLoadedModules();
    #endif // !defined(MFX_DISPATCHER_LOG)
#endif //(defined(LINUX32) || defined(LINUX64))
    msdk_printf(MSDK_STRING("\n"));

    return sts;
}
