#include <opendaq/credential_request_builder_impl.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/credential_request_factory.h>

BEGIN_NAMESPACE_OPENDAQ
CredentialRequestBuilderImpl::CredentialRequestBuilderImpl()
    : connectionString(nullptr)
    , componentType(nullptr)
    , metaData(PropertyObject())
{
}

ErrCode CredentialRequestBuilderImpl::build(ICredentialRequest** request)
{
    OPENDAQ_PARAM_NOT_NULL(request);

    const auto builderPtr = this->borrowPtr<CredentialRequestBuilderPtr>();

    return daqTry(
        [&]()
        {
            *request = CredentialRequestFromBuilder(builderPtr).detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode CredentialRequestBuilderImpl::setComponentType(IComponentType* componentType)
{
    this->componentType = componentType;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setConnectionString(IString* connectionString)
{
    this->connectionString = connectionString;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::addMetaDataProperty(IProperty* property)
{
    return metaData->addProperty(property);
}

ErrCode CredentialRequestBuilderImpl::getComponentType(IComponentType** componentType)
{
    OPENDAQ_PARAM_NOT_NULL(componentType);

    *componentType = this->componentType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getMetaData(IPropertyObject** metaData)
{
    OPENDAQ_PARAM_NOT_NULL(metaData);

    *metaData = this->metaData.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setPayloadId(IString* payloadId)
{
    this->payloadId = payloadId;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getPayloadId(IString** payloadId)
{
    OPENDAQ_PARAM_NOT_NULL(payloadId);

    *payloadId = this->payloadId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setPayloadDescriptor(ICredentialPayloadDescriptor* descriptor)
{
    this->payloadDescriptor = descriptor;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    *descriptor = this->payloadDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CredentialRequestBuilder, ICredentialRequestBuilder)

END_NAMESPACE_OPENDAQ
