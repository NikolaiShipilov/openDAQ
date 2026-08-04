#include <credential_demo_module/credential_demo_device_impl.h>
#include <credential_demo_module/credential_demo_module_impl.h>
#include <credential_demo_module/version.h>

#include <coretypes/version_info_factory.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

CredentialDemoModule::CredentialDemoModule(const ContextPtr& context)
    : Module(CREDENTIAL_DEMO_MODULE_NAME,
             VersionInfo(CREDENTIAL_DEMO_MODULE_MAJOR_VERSION,
                         CREDENTIAL_DEMO_MODULE_MINOR_VERSION,
                         CREDENTIAL_DEMO_MODULE_PATCH_VERSION),
             context,
             CREDENTIAL_DEMO_MODULE_ID)
{
}

ListPtr<IDeviceInfo> CredentialDemoModule::onGetAvailableDevices()
{
    return { CredentialDemoDeviceImpl::CreateDeviceInfo() };
}

DictPtr<IString, IDeviceType> CredentialDemoModule::onGetAvailableDeviceTypes()
{
    auto deviceType = CredentialDemoDeviceImpl::CreateType();
    return Dict<IString, IBaseObject>({{deviceType.getId(), deviceType}});
}

DevicePtr CredentialDemoModule::onCreateDevice(const StringPtr& connectionString,
                                               const ComponentPtr& parent,
                                               const PropertyObjectPtr& config)
{
    std::scoped_lock lock(sync);

    clearRemovedDevice();
    if (device.assigned())
        DAQ_THROW_EXCEPTION(AlreadyExistsException, "Credential demo device is already created!");

    auto info = CredentialDemoDeviceImpl::CreateDeviceInfo();

    DevicePtr devicePtr = createWithImplementation<IDevice, CredentialDemoDeviceImpl>(config, context, parent, info);
    device = devicePtr;
    return devicePtr.detach();
}

void CredentialDemoModule::clearRemovedDevice()
{
    const bool isNull = !device.assigned() || !device.getRef().assigned();
    if (isNull || device.getRef().isRemoved())
        device = nullptr;
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
