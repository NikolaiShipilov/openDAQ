#include <credential_demo_module/credential_demo_device_impl.h>
#include <credential_demo_module/credential_demo_module_impl.h>
#include <credential_demo_module/version.h>

#include <coretypes/version_info_factory.h>
#include <opendaq/credential_payload_descriptor_factory.h>

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

DevicePtr CredentialDemoModule::onCreateAuthenticatedDevice(const StringPtr& connectionString,
                                                            const StringPtr& manufacturer,
                                                            const StringPtr& serialNumber,
                                                            const ComponentPtr& parent,
                                                            const PropertyObjectPtr& config,
                                                            const AuthenticationConfigPtr& authenticationConfig)
{
    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(CREDENTIAL_DEMO_MODULE_ID));
    auto info = CredentialDemoDeviceImpl::CreateDeviceInfo(options);
    CredentialDemoDeviceImpl::ValidateConnectionString(connectionString);

    if (!authenticationConfig.assigned())
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Authentication is required but no authentication config was provided");
    }

    const auto payloadId = authenticationConfig.getCredentialPayloadId();
    const auto payloadDescriptor = authenticationConfig.getCredentialPayloadDescriptor();

    // The authenticated path always requests credentials - the device is never connected to anonymously.
    auto credentialProvider = FindMatchingCredentialProvider(context.getCredentialProviders(), payloadDescriptor);
    if (!credentialProvider.assigned())
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Authentication is required but no credential provider supporting a compatible payload format is registered");
    }

    const auto additionalConfig = authenticationConfig.getConfig();
    const bool verboseCredentialRequest = additionalConfig.getPropertyValue("VerboseCredentialRequest");

    const auto credentialRequest =
        payloadDescriptor.getFormat() == CredentialPayloadFormat::KeyValuePairs
            ? CredentialDemoDeviceImpl::CreateUserNamePasswordCredentialRequest(
                  connectionString, manufacturer, serialNumber, additionalConfig, verboseCredentialRequest)
            : CredentialDemoDeviceImpl::CreatePinCredentialRequest(
                  connectionString, manufacturer, serialNumber, additionalConfig, verboseCredentialRequest);

    return createWithImplementation<IDevice, CredentialDemoDeviceImpl>(
        config,
        context,
        parent,
        info,
        /*authenticated*/true,
        payloadId,
        credentialProvider.requestCredentials(credentialRequest)).detach();
}

CredentialProviderPtr CredentialDemoModule::FindMatchingCredentialProvider(const DictPtr<IString, ICredentialProvider>& providers,
                                                                           const CredentialPayloadDescriptorPtr& payloadDescriptor)
{
    for (const auto& [_, provider] : providers)
    {
        for (const auto& format : provider.getSupportedPayloadFormats())
        {
            if (static_cast<CredentialPayloadFormat>(static_cast<Int>(format)) == payloadDescriptor.getFormat())
                return provider;
        }
    }

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
