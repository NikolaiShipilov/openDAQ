#include <opendaq/credential_request_impl.h>
#include <opendaq/credential_request_builder_ptr.h>
#include <opendaq/credential_request_factory.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

DictPtr<IString, IBaseObject> CredentialRequestImpl::PackBuilder(ICredentialRequestBuilder* builder)
{
    const auto builderPtr = CredentialRequestBuilderPtr::Borrow(builder);
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentType", builderPtr.getComponentType());
    params.set("ConnectionString", builderPtr.getConnectionString());
    params.set("MetaData", builderPtr.getMetaData());
    params.set("Manufacturer", builderPtr.getManufacturer());
    params.set("SerialNumber", builderPtr.getSerialNumber());
    params.set("PayloadId", builderPtr.getPayloadId());
    params.set("PayloadDescriptor", builderPtr.getPayloadDescriptor());

    return params;
}

CredentialRequestImpl::CredentialRequestImpl(const DictPtr<IString, IBaseObject>& packedBuilder)
    : componentType(packedBuilder.get("ComponentType"))
    , connectionString(packedBuilder.get("ConnectionString"))
    , metaData(packedBuilder.get("MetaData"))
    , manufacturer(packedBuilder.get("Manufacturer"))
    , serialNumber(packedBuilder.get("SerialNumber"))
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

ErrCode CredentialRequestImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr CredentialRequestImpl::SerializeId()
{
    return "CredentialRequest";
}

ErrCode CredentialRequestImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    if (componentType.assigned())
    {
        serializer->key("ComponentType");
        componentType.serialize(serializer);
    }

    if (connectionString.assigned())
    {
        serializer->key("ConnectionString");
        serializer->writeString(connectionString.getCharPtr(), connectionString.getLength());
    }

    if (metaData.assigned())
    {
        serializer->key("MetaData");
        metaData.serialize(serializer);
    }

    if (manufacturer.assigned())
    {
        serializer->key("Manufacturer");
        serializer->writeString(manufacturer.getCharPtr(), manufacturer.getLength());
    }

    if (serialNumber.assigned())
    {
        serializer->key("SerialNumber");
        serializer->writeString(serialNumber.getCharPtr(), serialNumber.getLength());
    }

    if (payloadId.assigned())
    {
        serializer->key("PayloadId");
        serializer->writeString(payloadId.getCharPtr(), payloadId.getLength());
    }

    if (payloadDescriptor.assigned())
    {
        serializer->key("PayloadDescriptor");
        payloadDescriptor.serialize(serializer);
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            // Built directly as the packed-constructor dict rather than via `CredentialRequestBuilder`, so the
            // deserialized `MetaData` property object is adopted as-is instead of having its properties
            // re-added one by one - a property can only ever belong to a single owning property object, and
            // `addMetaDataProperty` would otherwise try to re-parent an already-owned property.
            auto packed = Dict<IString, IBaseObject>();
            packed.set("ComponentType", serializedObj.hasKey("ComponentType") ? serializedObj.readObject("ComponentType", contextPtr, factoryCallbackPtr) : BaseObjectPtr());
            packed.set("ConnectionString", serializedObj.hasKey("ConnectionString") ? serializedObj.readString("ConnectionString") : StringPtr());
            packed.set("MetaData", serializedObj.hasKey("MetaData") ? serializedObj.readObject("MetaData", contextPtr, factoryCallbackPtr) : BaseObjectPtr());
            packed.set("Manufacturer", serializedObj.hasKey("Manufacturer") ? serializedObj.readString("Manufacturer") : StringPtr());
            packed.set("SerialNumber", serializedObj.hasKey("SerialNumber") ? serializedObj.readString("SerialNumber") : StringPtr());
            packed.set("PayloadId", serializedObj.hasKey("PayloadId") ? serializedObj.readString("PayloadId") : StringPtr());
            packed.set("PayloadDescriptor", serializedObj.hasKey("PayloadDescriptor") ? serializedObj.readObject("PayloadDescriptor", contextPtr, factoryCallbackPtr) : BaseObjectPtr());

            *obj = createWithImplementation<ICredentialRequest, CredentialRequestImpl>(packed).detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, CredentialRequest,
    ICredentialRequest, createCredentialRequestFromBuilder,
    ICredentialRequestBuilder*, builder
)

END_NAMESPACE_OPENDAQ
