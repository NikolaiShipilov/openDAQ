#include <credential_demo_module/credential_demo_streaming_impl.h>

#include <opendaq/streaming_type_factory.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static const std::string CredentialDemoStreamingTypeId = "CredentialDemoStreaming";
static const std::string CredentialDemoStreamingPrefix = "daq.credential_demo_streaming";

CredentialDemoStreamingImpl::CredentialDemoStreamingImpl(const StringPtr& connectionString,
                                                          const ContextPtr& ctx,
                                                          const StringPtr& payloadId,
                                                          const PropertyObjectPtr& credentials)
    : Streaming(connectionString, ctx, /*skipDomainSignalSubscribe*/ true)
{
    authentication::Authenticate(ctx, credentials, payloadId);
}

StreamingTypePtr CredentialDemoStreamingImpl::CreateType()
{
    auto userNamePasswordDescriptor = authentication::BuildUserNamePasswordDescriptor();
    auto pinDescriptor = authentication::BuildPinDescriptor();
    auto privateKeyDescriptor = authentication::BuildPrivateKeyFileDescriptor();

    return StreamingTypeBuilder()
        .setId(CredentialDemoStreamingTypeId)
        .setName("Credential demo streaming")
        .setDescription("Dummy streaming connection, authenticated via the same credential framework and "
                         "auth methods as the device")
        .setConnectionStringPrefix(CredentialDemoStreamingPrefix)
        .addSupportedAuthenticationConfig(UserNamePasswordPayloadId, userNamePasswordDescriptor)
        .addSupportedAuthenticationConfig(PinPayloadId, pinDescriptor)
        .addSupportedAuthenticationConfig(PrivateKeyFilePayloadId, privateKeyDescriptor)
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
