#include <opendaq/credential_request_impl.h>
#include <opendaq/credential_request_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

DictPtr<IString, IBaseObject> CredentialRequestImpl::PackBuilder(ICredentialRequestBuilder* builder)
{
    const auto builderPtr = CredentialRequestBuilderPtr::Borrow(builder);
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentType", builderPtr.getComponentType());
    params.set("ConnectionString", builderPtr.getConnectionString());
    params.set("MetaData", builderPtr.getMetaData());
    params.set("PayloadId", builderPtr.getPayloadId());
    params.set("PayloadDescriptor", builderPtr.getPayloadDescriptor());

    return params;
}

CredentialRequestImpl::CredentialRequestImpl(const DictPtr<IString, IBaseObject>& packedBuilder)
    : connectionString(packedBuilder.get("ConnectionString"))
    , componentType(packedBuilder.get("ComponentType"))
    , metaData(packedBuilder.get("MetaData"))
    , payloadId(packedBuilder.get("PayloadId"))
    , payloadDescriptor(packedBuilder.get("PayloadDescriptor"))
{
}

CredentialRequestImpl::CredentialRequestImpl(ICredentialRequestBuilder* credentialRequestBuilder)
    : CredentialRequestImpl(PackBuilder(credentialRequestBuilder))
{
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
