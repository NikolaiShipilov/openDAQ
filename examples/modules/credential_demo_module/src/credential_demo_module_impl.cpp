#include <credential_demo_module/credential_demo_device_impl.h>
#include <credential_demo_module/credential_demo_module_impl.h>
#include <credential_demo_module/credential_demo_streaming_impl.h>
#include <credential_demo_module/version.h>

#include <coretypes/version_info_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <opendaq/credential_descriptor_factory.h>

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
    auto deviceType = CredentialDemoDeviceImpl::CreateType();
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
                                                            const StringPtr& authenticationMethodId,
                                                            const PropertyObjectPtr& credentials)
{
    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(CREDENTIAL_DEMO_MODULE_ID));
    auto info = CredentialDemoDeviceImpl::CreateDeviceInfo(options);
    CredentialDemoDeviceImpl::ValidateConnectionString(connectionString);

    // The device is never connected to anonymously via this path, only ever authenticated with the given
    // credentials.
    return createWithImplementation<IDevice, CredentialDemoDeviceImpl>(
        config, context, parent, info, /*authenticated*/true, authenticationMethodId, credentials).detach();
}

DictPtr<IString, IStreamingType> CredentialDemoModule::onGetAvailableStreamingTypes()
{
    auto streamingType = CredentialDemoStreamingImpl::CreateType();
    return Dict<IString, IBaseObject>({{streamingType.getId(), streamingType}});
}

StreamingPtr CredentialDemoModule::onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& /*config*/)
{
    // Only reached for the "Anonymous" method (or no authentication support at all, which never applies to
    // this module's own streaming type) - no real credentials to verify either way.
    return createWithImplementation<IStreaming, CredentialDemoStreamingImpl>(connectionString, context, StandardAnonymousId, nullptr);
}

StreamingPtr CredentialDemoModule::onCreateAuthenticatedStreaming(const StringPtr& connectionString,
                                                                   const PropertyObjectPtr& /*config*/,
                                                                   const StringPtr& authenticationMethodId,
                                                                   const PropertyObjectPtr& credentials)
{
    return createWithImplementation<IStreaming, CredentialDemoStreamingImpl>(connectionString, context, authenticationMethodId, credentials);
}

DictPtr<IString, ICredentialDescriptor> CredentialDemoModule::onGetSupportedAuthenticationMethods(const StringPtr& typeId)
{
    if (typeId != CredentialDemoDeviceImpl::CreateType().getId() && typeId != CredentialDemoStreamingImpl::CreateType().getId())
        return Dict<IString, ICredentialDescriptor>();

    auto userNamePasswordDescriptor = StandardUserNamePasswordCredentialDescriptor(context.getTypeManager());
    auto pinDescriptor = StandardPinCredentialDescriptor(context.getTypeManager());
    auto privateKeyDescriptor = StandardPrivateKeyFileCredentialDescriptor(context.getTypeManager());
    auto anonymousDescriptor = StandardAnonymousCredentialDescriptor(context.getTypeManager());

    return Dict<IString, ICredentialDescriptor>({{userNamePasswordDescriptor.getAuthenticationMethodId(), userNamePasswordDescriptor},
                                                        {pinDescriptor.getAuthenticationMethodId(), pinDescriptor},
                                                        {privateKeyDescriptor.getAuthenticationMethodId(), privateKeyDescriptor},
                                                        {anonymousDescriptor.getAuthenticationMethodId(), anonymousDescriptor}});
}

StringPtr CredentialDemoModule::onGetDefaultAuthenticationMethodId(const StringPtr& typeId)
{
    if (typeId == CredentialDemoDeviceImpl::CreateType().getId())
        return StandardUserNamePasswordId;
    if (typeId == CredentialDemoStreamingImpl::CreateType().getId())
        return StandardPinId;

    return nullptr;
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
