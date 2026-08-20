#include <credential_demo_module/credential_demo_streaming_impl.h>

#include <opendaq/streaming_type_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static const std::string CredentialDemoStreamingTypeId = "CredentialDemoStreaming";
static const std::string CredentialDemoStreamingPrefix = "daq.credential_demo_streaming";

CredentialDemoStreamingImpl::CredentialDemoStreamingImpl(const StringPtr& connectionString,
                                                          const ContextPtr& ctx,
                                                          const StringPtr& payloadId,
                                                          const CredentialPayloadPtr& credentials)
    : Streaming(connectionString, ctx, /*skipDomainSignalSubscribe*/ true)
{
    authentication::Authenticate(ctx, credentials, payloadId);
}

StreamingTypePtr CredentialDemoStreamingImpl::CreateType()
{
    auto userNamePasswordDescriptor = authentication::BuildUserNamePasswordDescriptor(/*hidePassword*/ true);
    auto pinDescriptor = authentication::BuildPinDescriptor(/*hidePin*/ true);
    auto privateKeyDescriptor = authentication::BuildPrivateKeyFileDescriptor();
    auto privateKeyBlobDescriptor = authentication::BuildPrivateKeyBlobDescriptor();

    // `IDevice::addStreaming`/`IModule::createStreaming` - unlike `addAuthenticatedDevice` - carry no
    // separate authentication-config parameter, so the chosen payload id has nowhere to travel except
    // inside the plain config object itself; each supported config below carries its own fixed
    // "PayloadId" alongside the same knobs the device's equivalent config has.
    auto userNamePasswordConfig = authentication::BuildAdditionalConfig(UserNamePasswordPayloadId);
    userNamePasswordConfig.addProperty(StringProperty("PayloadId", UserNamePasswordPayloadId));

    auto pinConfig = authentication::BuildAdditionalConfig(PinPayloadId);
    pinConfig.addProperty(StringProperty("PayloadId", PinPayloadId));

    auto privateKeyConfig = authentication::BuildAdditionalConfig(PrivateKeyFilePayloadId);
    privateKeyConfig.addProperty(StringProperty("PayloadId", PrivateKeyFilePayloadId));

    auto privateKeyBlobConfig = authentication::BuildAdditionalConfig(PrivateKeyBlobPayloadId);
    privateKeyBlobConfig.addProperty(StringProperty("PayloadId", PrivateKeyBlobPayloadId));

    // `Module::createStreaming` merges whatever config the caller passes into a copy of the type's
    // default config (`mergeConfig`/`populateDefaultConfig` only carry over properties the default config
    // already declares - anything else gets silently dropped). So the default config's schema has to be
    // the union of every supported config's properties, or the caller's chosen "PayloadId"/knobs would
    // never survive to reach `onCreateStreaming`.
    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(StringProperty("PayloadId", PinPayloadId));
    defaultConfig.addProperty(BoolProperty("VerboseCredentialRequest", False));
    defaultConfig.addProperty(BoolProperty("HidePasswordInput", True));
    defaultConfig.addProperty(BoolProperty("HidePinInput", True));

    return StreamingTypeBuilder()
        .setId(CredentialDemoStreamingTypeId)
        .setName("Credential demo streaming")
        .setDescription("Dummy streaming connection, authenticated via the same credential framework and "
                         "auth methods as the device")
        .setConnectionStringPrefix(CredentialDemoStreamingPrefix)
        .setDefaultConfig(defaultConfig)
        .addSupportedAuthenticationConfig(UserNamePasswordPayloadId, userNamePasswordDescriptor, userNamePasswordConfig)
        .addSupportedAuthenticationConfig(PinPayloadId, pinDescriptor, pinConfig)
        .addSupportedAuthenticationConfig(PrivateKeyFilePayloadId, privateKeyDescriptor, privateKeyConfig)
        .addSupportedAuthenticationConfig(PrivateKeyBlobPayloadId, privateKeyBlobDescriptor, privateKeyBlobConfig)
        .setDefaultAuthenticationConfigId(PinPayloadId)
        .build();
}

void CredentialDemoStreamingImpl::onSetActive(bool /*active*/)
{
}

void CredentialDemoStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void CredentialDemoStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void CredentialDemoStreamingImpl::onSubscribeSignal(const StringPtr& /*signalStreamingId*/)
{
}

void CredentialDemoStreamingImpl::onUnsubscribeSignal(const StringPtr& /*signalStreamingId*/)
{
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
