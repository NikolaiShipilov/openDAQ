#include <opendaq/credential_request_builder_impl.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/credential_request_factory.h>

BEGIN_NAMESPACE_OPENDAQ
CredentialRequestBuilderImpl::CredentialRequestBuilderImpl()
    : connectionString(nullptr)
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

ErrCode CredentialRequestBuilderImpl::setConnectionString(IString* connectionString)
{
    this->connectionString = connectionString;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::addMetaDataProperty(IProperty* property)
{
    return metaData->addProperty(property);
}

ErrCode CredentialRequestBuilderImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setManufacturer(IString* manufacturer)
{
    this->manufacturer = manufacturer;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getManufacturer(IString** manufacturer)
{
    OPENDAQ_PARAM_NOT_NULL(manufacturer);

    *manufacturer = this->manufacturer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setSerialNumber(IString* serialNumber)
{
    this->serialNumber = serialNumber;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getSerialNumber(IString** serialNumber)
{
    OPENDAQ_PARAM_NOT_NULL(serialNumber);

    *serialNumber = this->serialNumber.addRefAndReturn();
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
