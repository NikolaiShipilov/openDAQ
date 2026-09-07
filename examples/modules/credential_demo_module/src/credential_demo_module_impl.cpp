#include <credential_demo_module/credential_demo_device_impl.h>
#include <credential_demo_module/credential_demo_module_impl.h>
#include <credential_demo_module/credential_demo_streaming_impl.h>
#include <credential_demo_module/version.h>

#include <coretypes/version_info_factory.h>
#include <coretypes/stringobject_factory.h>

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
    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(CREDENTIAL_DEMO_MODULE_ID));
    return { CredentialDemoDeviceImpl::CreateDeviceInfo(options) };
}

DictPtr<IString, IDeviceType> CredentialDemoModule::onGetAvailableDeviceTypes()
{
    auto deviceType = CredentialDemoDeviceImpl::CreateType(context.getTypeManager());
    return Dict<IString, IBaseObject>({{deviceType.getId(), deviceType}});
}

DevicePtr CredentialDemoModule::onCreateDevice(const StringPtr& connectionString,
                                               const ComponentPtr& parent,
                                               const PropertyObjectPtr& config)
{
    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(CREDENTIAL_DEMO_MODULE_ID));
    auto info = CredentialDemoDeviceImpl::CreateDeviceInfo(options);
    CredentialDemoDeviceImpl::ValidateConnectionString(connectionString);

    // The plain, non-authenticated path doesn't request credentials - the device is "connected" to anonymously.
    return createWithImplementation<IDevice, CredentialDemoDeviceImpl>(config, context, parent, info, /*authenticated*/false).detach();
}

StringPtr CredentialDemoModule::onGetCanonicalConnectionString(const StringPtr& connectionString)
{
    // Only the routing prefix ever gets trimmed here - there's nothing left to make explicit beyond that,
    // since this module's connection strings never leave any parameter (host, port, path, ...) unspecified
    // in the first place: each type has exactly one, fixed, non-parameterized address.
    const std::string connStr = connectionString;

    const std::string devicePrefix = CredentialDemoDeviceImpl::CreateType().getConnectionStringPrefix().toStdString() + "://";
    if (connStr.rfind(devicePrefix, 0) == 0)
        return String(connStr.substr(devicePrefix.size()));

    const std::string streamingPrefix = CredentialDemoStreamingImpl::CreateType().getConnectionStringPrefix().toStdString() + "://";
    if (connStr.rfind(streamingPrefix, 0) == 0)
        return String(connStr.substr(streamingPrefix.size()));

    return connectionString;
}

DevicePtr CredentialDemoModule::onCreateAuthenticatedDevice(const StringPtr& connectionString,
                                                            const ComponentPtr& parent,
                                                            const PropertyObjectPtr& config,
                                                            const StringPtr& payloadId,
                                                            const PropertyObjectPtr& credentials)
{
    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(CREDENTIAL_DEMO_MODULE_ID));
    auto info = CredentialDemoDeviceImpl::CreateDeviceInfo(options);
    CredentialDemoDeviceImpl::ValidateConnectionString(connectionString);

    // `createAuthenticatedDevice` (in `Module`) has already resolved `credentials` - the device is never
    // connected to anonymously via this path, only ever authenticated with what was already obtained.
    return createWithImplementation<IDevice, CredentialDemoDeviceImpl>(
        config, context, parent, info, /*authenticated*/true, payloadId, credentials).detach();
}

DictPtr<IString, IStreamingType> CredentialDemoModule::onGetAvailableStreamingTypes()
{
    auto streamingType = CredentialDemoStreamingImpl::CreateType(context.getTypeManager());
    return Dict<IString, IBaseObject>({{streamingType.getId(), streamingType}});
}

StreamingPtr CredentialDemoModule::onCreateStreaming(const StringPtr& connectionString,
                                                     const PropertyObjectPtr& /*config*/,
                                                     const StringPtr& payloadId,
                                                     const PropertyObjectPtr& credentials)
{
    // `createStreaming` (in `Module`) has already resolved `credentials` - even when the caller left
    // authentication unspecified, since this streaming type declares a default authentication method
    // (`CredentialDemoStreamingImpl::CreateType()::getDefaultAuthenticationConfigId()`), so
    // `resolveDefaultAuthenticationConfig` falls back to it rather than skipping authentication.
    return createWithImplementation<IStreaming, CredentialDemoStreamingImpl>(connectionString, context, payloadId, credentials);
}

DictPtr<IString, IBaseObject> CredentialDemoModule::populateDefaultModuleOptions(const DictPtr<IString, IBaseObject>& inputOptions)
{
    auto defaultOptions = Dict<IString, IBaseObject>({{"Manufacturer", "openDAQ"}, {"SerialNumber", "0"}});

    for (const auto& [key, value] : inputOptions)
    {
        if (defaultOptions.hasKey(key))
        {
            defaultOptions.set(key, value);
        }
    }

    return defaultOptions;
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
