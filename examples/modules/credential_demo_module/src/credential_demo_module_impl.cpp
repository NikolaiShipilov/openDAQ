#include <credential_demo_module/credential_demo_device_impl.h>
#include <credential_demo_module/credential_demo_module_impl.h>
#include <credential_demo_module/credential_demo_streaming_impl.h>
#include <credential_demo_module/credential_demo_authenticator.h>
#include <credential_demo_module/version.h>

#include <coretypes/version_info_factory.h>
#include <opendaq/credential_payload_descriptor_factory.h>
#include <opendaq/authentication_config_factory.h>
#include <opendaq/authentication_config_private_ptr.h>
#include <opendaq/component_private_ptr.h>

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
    auto credentialProvider =
        FindMatchingCredentialProvider(context.getCredentialProviders(), payloadDescriptor, authenticationConfig.getCredentialProviderId());
    if (!credentialProvider.assigned())
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Authentication is required but no credential provider supporting a compatible payload format is registered");
    }

    // A config reconstructed while reloading a saved device already carries the request formed the first
    // time around - reuse it as-is instead of forming a new one from the payload descriptor and additional
    // config.
    auto credentialRequest = authenticationConfig.asPtr<IAuthenticationConfigPrivate>(true).getCredentialRequest();
    if (!credentialRequest.assigned())
    {
        const auto additionalConfig = authenticationConfig.getConfig();
        const bool verboseCredentialRequest = additionalConfig.getPropertyValue("VerboseCredentialRequest");

        credentialRequest = authentication::CreateCredentialRequest(
            payloadId, connectionString, manufacturer, serialNumber, additionalConfig, verboseCredentialRequest, CredentialDemoDeviceImpl::CreateType());
    }

    auto device = createWithImplementation<IDevice, CredentialDemoDeviceImpl>(
        config,
        context,
        parent,
        info,
        /*authenticated*/true,
        payloadId,
        credentialProvider.requestCredentials(credentialRequest),
        authenticationConfig);

    // Persisted alongside the device, so a reload can re-request credentials for it without ever having
    // saved the authentication config or its secrets.
    if (const auto& componentPrivate = device.asPtrOrNull<IComponentPrivate>(true); componentPrivate.assigned())
        componentPrivate.setCredentialRequest(credentialRequest);

    return device.detach();
}

DictPtr<IString, IStreamingType> CredentialDemoModule::onGetAvailableStreamingTypes()
{
    auto streamingType = CredentialDemoStreamingImpl::CreateType();
    return Dict<IString, IBaseObject>({{streamingType.getId(), streamingType}});
}

StreamingPtr CredentialDemoModule::onCreateStreaming(const StringPtr& connectionString,
                                                     const PropertyObjectPtr& config,
                                                     const AuthenticationConfigPtr& authenticationConfig,
                                                     const StringPtr& manufacturer,
                                                     const StringPtr& serialNumber)
{
    // The automatic streaming-attach path (`addDevice`'s "PrioritizedStreamingProtocols" config) has no way
    // to supply an authentication config - it always calls through with a null one. Rather than failing, fall
    // back to the streaming type's own default authentication method instead of requiring an explicit one.
    auto resolvedAuthenticationConfig = authenticationConfig;
    if (!resolvedAuthenticationConfig.assigned())
        resolvedAuthenticationConfig = CredentialDemoStreamingImpl::CreateType().createDefaultAuthenticationConfig();

    const auto payloadId = resolvedAuthenticationConfig.getCredentialPayloadId();
    const auto payloadDescriptor = resolvedAuthenticationConfig.getCredentialPayloadDescriptor();

    auto credentialProvider = FindMatchingCredentialProvider(
        context.getCredentialProviders(), payloadDescriptor, resolvedAuthenticationConfig.getCredentialProviderId());
    if (!credentialProvider.assigned())
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                             "Streaming authentication is required but no credential provider supporting a compatible payload format is registered");
    }

    const bool verboseCredentialRequest = config.getPropertyValue("VerboseCredentialRequest");

    const auto credentialRequest = authentication::CreateCredentialRequest(
        payloadId, connectionString, manufacturer, serialNumber, config, verboseCredentialRequest, CredentialDemoStreamingImpl::CreateType());

    const auto credentials = credentialProvider.requestCredentials(credentialRequest);

    return createWithImplementation<IStreaming, CredentialDemoStreamingImpl>(connectionString, context, payloadId, credentials);
}

static bool SupportsPayloadFormat(const CredentialProviderPtr& provider, const CredentialPayloadDescriptorPtr& payloadDescriptor)
{
    for (const auto& format : provider.getSupportedPayloadFormats())
    {
        if (static_cast<CredentialPayloadFormat>(static_cast<Int>(format)) == payloadDescriptor.getFormat())
            return true;
    }

    return false;
}

CredentialProviderPtr CredentialDemoModule::FindMatchingCredentialProvider(const DictPtr<IString, ICredentialProvider>& providers,
                                                                           const CredentialPayloadDescriptorPtr& payloadDescriptor,
                                                                           const StringPtr& providerId)
{
    // An explicitly selected provider id still has to support the required payload format - it is not used
    // blindly just because it was named explicitly. An id that names no registered provider at all, or one
    // that doesn't support the required format, is failed here directly with a message naming the problem,
    // instead of falling through to the generic "no compatible provider" error below (which only applies to
    // auto-selection).
    if (providerId.assigned())
    {
        if (!providers.assigned() || !providers.hasKey(providerId))
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                 "Authentication is required but the explicitly selected credential provider \"{}\" is not registered",
                                 providerId);
        }

        auto provider = providers.get(providerId);
        if (!SupportsPayloadFormat(provider, payloadDescriptor))
        {
            DAQ_THROW_EXCEPTION(
                AuthenticationFailedException,
                "Authentication is required but the explicitly selected credential provider \"{}\" does not support the required payload format",
                providerId);
        }

        return provider;
    }

    for (const auto& [_, provider] : providers)
    {
        if (SupportsPayloadFormat(provider, payloadDescriptor))
            return provider;
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
