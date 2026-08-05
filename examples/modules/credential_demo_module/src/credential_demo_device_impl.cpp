#include <credential_demo_module/credential_demo_device_impl.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/credential_request_factory.h>
#include <fmt/format.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

CredentialDemoDeviceImpl::CredentialDemoDeviceImpl(const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const DeviceInfoPtr& info, const CredentialPayloadPtr& credentials)
    : Device(ctx, parent, fmt::format("{}_{}", info.getManufacturer(), info.getSerialNumber()), nullptr, info.getName())
{
    if (!credentials.assigned())
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate device - no credentials provided");
    }

    DictPtr<IString, IBaseObject> usernameAndPassword = credentials.getSecrets();
    if (!usernameAndPassword.assigned() ||
        !usernameAndPassword.hasKey("UserName") || usernameAndPassword.get("UserName") != "user" ||
        !usernameAndPassword.hasKey("Password") || usernameAndPassword.get("Password") != "aaa")
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate device - wrong credentials");
    }

    this->deviceInfo = info;
}

DeviceInfoPtr CredentialDemoDeviceImpl::CreateDeviceInfo(const DictPtr<IString, IBaseObject>& moduleOptions)
{
    const StringPtr manufacturer = moduleOptions.get("Manufacturer");
    const StringPtr serialNumber = moduleOptions.get("SerialNumber");

    auto connectionString = fmt::format("daq.credential_demo://{}_{}", manufacturer, serialNumber);
    auto devInfo = DeviceInfo(connectionString);
    devInfo.setName("Credential demo device");
    devInfo.setManufacturer(manufacturer);
    devInfo.setModel("Credential demo device");
    devInfo.setSerialNumber(serialNumber);
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

CredentialRequestPtr CredentialDemoDeviceImpl::CreateCredentialRequest(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    auto builder = CredentialRequestBuilder();
    return builder.setComponentType(CreateType()).setConnectionString(connectionString).build();
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
