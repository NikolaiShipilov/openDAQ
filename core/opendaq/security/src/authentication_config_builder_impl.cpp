#include <opendaq/authentication_config_builder_impl.h>
#include <opendaq/authentication_config_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigBuilderImpl::AuthenticationConfigBuilderImpl()
{
}

ErrCode AuthenticationConfigBuilderImpl::build(IAuthenticationConfig** authenticationConfig)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationConfig);

    return daqTry(
        [&]()
        {
            // A single-descriptor config is just the one-entry case of the underlying dict-keyed
            // constructor - `payloadDescriptor` left unassigned yields an empty dict, so the constructor's
            // own "at least one payload descriptor" check reports the error uniformly.
            auto payloadDescriptors = Dict<IString, ICredentialPayloadDescriptor>();
            StringPtr defaultPayloadId;
            if (payloadDescriptor.assigned())
            {
                defaultPayloadId = payloadDescriptor.getId();
                payloadDescriptors.set(defaultPayloadId, payloadDescriptor);
            }

            // `AuthenticationConfigBuilder` has no `Context` access to enumerate credential providers, so it
            // never supplies a provider id list - the built config's "CredentialProviderId" property is
            // omitted entirely (see `AuthenticationConfigImpl::initProperties`), same as leaving
            // `credentialProviderId` unset would already mean today.
            *authenticationConfig =
                createWithImplementation<IAuthenticationConfig, AuthenticationConfigImpl>(
                    payloadDescriptors, defaultPayloadId, nullptr, credentialProviderId, suppliedSecret)
                    .detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode AuthenticationConfigBuilderImpl::setPayloadDescriptor(ICredentialPayloadDescriptor* descriptor)
{
    this->payloadDescriptor = descriptor;
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigBuilderImpl::getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    *descriptor = this->payloadDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigBuilderImpl::setCredentialProviderId(IString* providerId)
{
    this->credentialProviderId = providerId;
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigBuilderImpl::getCredentialProviderId(IString** providerId)
{
    OPENDAQ_PARAM_NOT_NULL(providerId);

    *providerId = this->credentialProviderId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigBuilderImpl::setSuppliedSecret(IPropertyObject* suppliedSecret)
{
    this->suppliedSecret = suppliedSecret;
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigBuilderImpl::getSuppliedSecret(IPropertyObject** suppliedSecret)
{
    OPENDAQ_PARAM_NOT_NULL(suppliedSecret);

    *suppliedSecret = this->suppliedSecret.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfigBuilder, IAuthenticationConfigBuilder
)

END_NAMESPACE_OPENDAQ
