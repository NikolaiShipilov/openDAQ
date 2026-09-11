#include <opendaq/credential_request_impl.h>
#include <opendaq/credential_request_builder_ptr.h>
#include <opendaq/credential_request_factory.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

CredentialRequestImpl::CredentialRequestImpl(ICredentialRequestBuilder* credentialRequestBuilder)
{
    const auto builderPtr = CredentialRequestBuilderPtr::Borrow(credentialRequestBuilder);
    componentType = builderPtr.getComponentType();
    connectionString = builderPtr.getConnectionString();
    metaData = builderPtr.getMetaData();
    manufacturer = builderPtr.getManufacturer();
    serialNumber = builderPtr.getSerialNumber();
    payloadId = builderPtr.getPayloadId();
    payloadDescriptor = builderPtr.getPayloadDescriptor();

    if (!componentType.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Component type must be assigned when creating a credential request");

    if (!connectionString.assigned() || connectionString.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Connection string must be assigned when creating a credential request");

    if (!payloadId.assigned() || payloadId.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Payload id must be assigned when creating a credential request");

    if (!payloadDescriptor.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Payload descriptor must be assigned when creating a credential request");
}

ErrCode CredentialRequestImpl::getComponentType(IComponentType** componentType)
{
    OPENDAQ_PARAM_NOT_NULL(componentType);

    *componentType = this->componentType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getMetaData(IPropertyObject** metaData)
{
    OPENDAQ_PARAM_NOT_NULL(metaData);

    *metaData = this->metaData.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getManufacturer(IString** manufacturer)
{
    OPENDAQ_PARAM_NOT_NULL(manufacturer);

    *manufacturer = this->manufacturer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getSerialNumber(IString** serialNumber)
{
    OPENDAQ_PARAM_NOT_NULL(serialNumber);

    *serialNumber = this->serialNumber.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getPayloadId(IString** payloadId)
{
    OPENDAQ_PARAM_NOT_NULL(payloadId);

    *payloadId = this->payloadId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    *descriptor = this->payloadDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, CredentialRequest,
    ICredentialRequest, createCredentialRequestFromBuilder,
    ICredentialRequestBuilder*, builder
)

END_NAMESPACE_OPENDAQ
