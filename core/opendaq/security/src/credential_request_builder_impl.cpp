#include <opendaq/credential_request_builder_impl.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/credential_request_factory.h>

BEGIN_NAMESPACE_OPENDAQ
CredentialRequestBuilderImpl::CredentialRequestBuilderImpl()
    : componentType(nullptr)
    , connectionString(nullptr)
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

ErrCode CredentialRequestBuilderImpl::getComponentType(IComponentType** componentType)
{
    OPENDAQ_PARAM_NOT_NULL(componentType);

    *componentType = this->componentType.addRefAndReturn();
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

ErrCode CredentialRequestBuilderImpl::setAuthenticationMethodId(IString* authenticationMethodId)
{
    this->authenticationMethodId = authenticationMethodId;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getAuthenticationMethodId(IString** authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    *authenticationMethodId = this->authenticationMethodId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setDescriptor(ICredentialDescriptor* descriptor)
{
    this->descriptor = descriptor;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getDescriptor(ICredentialDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    *descriptor = this->descriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CredentialRequestBuilder, ICredentialRequestBuilder)

END_NAMESPACE_OPENDAQ
