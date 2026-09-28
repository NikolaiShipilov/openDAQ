#include <credential_demo_module/credential_demo_streaming_impl.h>

#include <opendaq/streaming_type_factory.h>
#include <opendaq/credential_descriptor_factory.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static const std::string CredentialDemoStreamingTypeId = "CredentialDemoStreaming";

CredentialDemoStreamingImpl::CredentialDemoStreamingImpl(const StringPtr& connectionString,
                                                          const ContextPtr& ctx,
                                                          const StringPtr& authenticationMethodId,
                                                          const PropertyObjectPtr& credentials)
    : Streaming(connectionString, ctx, /*skipDomainSignalSubscribe*/ true)
{
    authentication::Authenticate(ctx, credentials, authenticationMethodId);
}

StreamingTypePtr CredentialDemoStreamingImpl::CreateType(const ContextPtr& context)
{
    auto userNamePasswordDescriptor = StandardUserNamePasswordCredentialDescriptor(context.getTypeManager());
    auto pinDescriptor = StandardPinCredentialDescriptor(context.getTypeManager());
    auto privateKeyDescriptor = StandardPrivateKeyFileCredentialDescriptor(context.getTypeManager());
    auto anonymousDescriptor = StandardAnonymousCredentialDescriptor();

    // Showcases the same four authentication methods as the device, defaulting to PIN.
    auto supportedMethods =
        Dict<IString, ICredentialDescriptor>({{userNamePasswordDescriptor.getAuthenticationMethodId(), userNamePasswordDescriptor},
                                              {pinDescriptor.getAuthenticationMethodId(), pinDescriptor},
                                              {privateKeyDescriptor.getAuthenticationMethodId(), privateKeyDescriptor},
                                              {anonymousDescriptor.getAuthenticationMethodId(), anonymousDescriptor}});

    return StreamingTypeBuilder()
        .setId(CredentialDemoStreamingTypeId)
        .setName("Credential demo streaming")
        .setDescription("Dummy streaming connection, authenticated via the same credential framework and "
                         "auth methods as the device")
        .setConnectionStringPrefix(Prefix)
        .setSupportedAuthenticationMethods(supportedMethods)
        .setDefaultAuthenticationMethodId(StandardPinId)
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
