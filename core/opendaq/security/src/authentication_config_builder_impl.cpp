#include <opendaq/authentication_config_builder_impl.h>
#include <opendaq/authentication_config_impl.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigBuilderImpl::AuthenticationConfigBuilderImpl()
    : config(PropertyObject())
    , streamingAuthenticationConfigs(Dict<IString, IAuthenticationConfig>())
{
}

ErrCode AuthenticationConfigBuilderImpl::build(IAuthenticationConfig** authenticationConfig)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationConfig);

    return daqTry(
        [&]()
        {
            *authenticationConfig =
                createWithImplementation<IAuthenticationConfig, AuthenticationConfigImpl>(
                    payloadId, payloadDescriptor, config, streamingAuthenticationConfigs, credentialProviderId)
                    .detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode AuthenticationConfigBuilderImpl::setPayloadId(IString* payloadId)
{
    this->payloadId = payloadId;
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigBuilderImpl::getPayloadId(IString** payloadId)
{
    OPENDAQ_PARAM_NOT_NULL(payloadId);

    *payloadId = this->payloadId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
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

ErrCode AuthenticationConfigBuilderImpl::setConfig(IPropertyObject* config)
{
    this->config = config;
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigBuilderImpl::getConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    *config = this->config.addRefAndReturn();
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

ErrCode AuthenticationConfigBuilderImpl::addStreamingAuthenticationConfig(IStreamingType* streamingType, IAuthenticationConfig* streamingAuthenticationConfig)
{
    OPENDAQ_PARAM_NOT_NULL(streamingType);
    OPENDAQ_PARAM_NOT_NULL(streamingAuthenticationConfig);

    const StringPtr typeId = StreamingTypePtr::Borrow(streamingType).getId();
    streamingAuthenticationConfigs.set(typeId, streamingAuthenticationConfig);
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigBuilderImpl::getStreamingAuthenticationConfigs(IDict** streamingAuthenticationConfigs)
{
    OPENDAQ_PARAM_NOT_NULL(streamingAuthenticationConfigs);

    *streamingAuthenticationConfigs = this->streamingAuthenticationConfigs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfigBuilder, IAuthenticationConfigBuilder
)

END_NAMESPACE_OPENDAQ
