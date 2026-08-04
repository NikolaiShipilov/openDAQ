#include <credential_demo_module/credential_demo_device_impl.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

CredentialDemoDeviceImpl::CredentialDemoDeviceImpl(const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const DeviceInfoPtr& info)
    : Device(ctx, parent, "credential_demo_device", nullptr, info.getName())
{
    this->deviceInfo = info;
}

DeviceInfoPtr CredentialDemoDeviceImpl::CreateDeviceInfo()
{
    auto devInfo = DeviceInfo("daq.credential_demo://credential_demo_device");
    devInfo.setName("Credential demo device");
    devInfo.setManufacturer("openDAQ");
    devInfo.setModel("Credential demo device");
    devInfo.setSerialNumber("0");
    devInfo.setDeviceType(CreateType());

    return devInfo;
}

DeviceTypePtr CredentialDemoDeviceImpl::CreateType()
{
    return DeviceType("CredentialDemoDevice",
                      "Credential demo device",
                      "openDAQ authentication/credential framework showcase device",
                      "daq.credential_demo");
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
