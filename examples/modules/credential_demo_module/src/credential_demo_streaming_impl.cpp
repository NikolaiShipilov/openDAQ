#include <credential_demo_module/credential_demo_streaming_impl.h>

#include <opendaq/streaming_type_factory.h>
#include <opendaq/authentication_method_factory.h>
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
    auto userNamePasswordMethod = StandardUserNamePasswordAuthenticationMethod(context.getTypeManager());
    auto pinMethod = StandardPinAuthenticationMethod(context.getTypeManager());
    auto privateKeyMethod = StandardPrivateKeyFileAuthenticationMethod(context.getTypeManager());
    auto anonymousMethod = StandardAnonymousAuthenticationMethod();

    // Showcases the same four authentication methods as the device, defaulting to PIN.
    auto supportedMethods =
        Dict<IString, IAuthenticationMethod>({{userNamePasswordMethod.getId(), userNamePasswordMethod},
                                              {pinMethod.getId(), pinMethod},
                                              {privateKeyMethod.getId(), privateKeyMethod},
                                              {anonymousMethod.getId(), anonymousMethod}});

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
