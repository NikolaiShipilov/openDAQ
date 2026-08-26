#include <opendaq/authentication_config_impl.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(const StringPtr& payloadId,
                                                   const CredentialPayloadDescriptorPtr& payloadDescriptor,
                                                   const PropertyObjectPtr& config,
                                                   const DictPtr<IString, IAuthenticationConfig>& streamingAuthenticationConfigs,
                                                   const StringPtr& credentialProviderId,
                                                   const PropertyObjectPtr& suppliedSecret)
    : payloadId(payloadId)
    , payloadDescriptor(payloadDescriptor)
    , config(config.assigned() ? config : PropertyObject())
    , streamingAuthenticationConfigs(streamingAuthenticationConfigs.assigned() ? streamingAuthenticationConfigs : Dict<IString, IAuthenticationConfig>())
    , credentialProviderId(credentialProviderId)
    , suppliedSecret(suppliedSecret)
{
}

AuthenticationConfigImpl::AuthenticationConfigImpl(const CredentialRequestPtr& credentialRequest)
    : config(PropertyObject())
    , credentialRequest(credentialRequest)
    , streamingAuthenticationConfigs(Dict<IString, IAuthenticationConfig>())
{
    if (!credentialRequest.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Credential request must be assigned when reconstructing an authentication config from it");

    payloadId = credentialRequest.getPayloadId();
    payloadDescriptor = credentialRequest.getPayloadDescriptor();
}

ErrCode AuthenticationConfigImpl::getCredentialPayloadId(IString** payloadId)
{
    OPENDAQ_PARAM_NOT_NULL(payloadId);

    *payloadId = this->payloadId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    *descriptor = this->payloadDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::getConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    *config = this->config.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::getCredentialProviderId(IString** providerId)
{
    OPENDAQ_PARAM_NOT_NULL(providerId);

    *providerId = this->credentialProviderId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::getSuppliedSecret(IPropertyObject** suppliedSecret)
{
    OPENDAQ_PARAM_NOT_NULL(suppliedSecret);

    *suppliedSecret = this->suppliedSecret.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::getStreamingAuthenticationConfigs(IDict** streamingAuthenticationConfigs)
{
    OPENDAQ_PARAM_NOT_NULL(streamingAuthenticationConfigs);

    *streamingAuthenticationConfigs = this->streamingAuthenticationConfigs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::getCredentialRequest(ICredentialRequest** request)
{
    OPENDAQ_PARAM_NOT_NULL(request);

    *request = this->credentialRequest.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IString*, payloadId, ICredentialPayloadDescriptor*, payloadDescriptor, IPropertyObject*, config
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, AuthenticationConfig,
    IAuthenticationConfig, createAuthenticationConfigFromCredentialRequest,
    ICredentialRequest*, credentialRequest
)

END_NAMESPACE_OPENDAQ
