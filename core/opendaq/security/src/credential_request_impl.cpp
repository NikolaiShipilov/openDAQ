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
    : connectionString(packedBuilder.get("ConnectionString"))
    , componentType(packedBuilder.get("ComponentType"))
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

    if (connectionString.assigned())
    {
        serializer->key("ConnectionString");
        serializer->writeString(connectionString.getCharPtr(), connectionString.getLength());
    }

    if (componentType.assigned())
    {
        serializer->key("ComponentType");
        componentType.serialize(serializer);
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
            auto builder = CredentialRequestBuilder();

            if (serializedObj.hasKey("ConnectionString"))
                builder.setConnectionString(serializedObj.readString("ConnectionString"));

            if (serializedObj.hasKey("ComponentType"))
                builder.setComponentType(serializedObj.readObject("ComponentType", contextPtr, factoryCallbackPtr));

            if (serializedObj.hasKey("MetaData"))
            {
                const PropertyObjectPtr metaData = serializedObj.readObject("MetaData", contextPtr, factoryCallbackPtr);
                for (const auto& property : metaData.getAllProperties())
                    builder.addMetaDataProperty(property);
            }

            if (serializedObj.hasKey("Manufacturer"))
                builder.setManufacturer(serializedObj.readString("Manufacturer"));

            if (serializedObj.hasKey("SerialNumber"))
                builder.setSerialNumber(serializedObj.readString("SerialNumber"));

            if (serializedObj.hasKey("PayloadId"))
                builder.setPayloadId(serializedObj.readString("PayloadId"));

            if (serializedObj.hasKey("PayloadDescriptor"))
                builder.setPayloadDescriptor(serializedObj.readObject("PayloadDescriptor", contextPtr, factoryCallbackPtr));

            *obj = builder.build().detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, CredentialRequest,
    ICredentialRequest, createCredentialRequestFromBuilder,
    ICredentialRequestBuilder*, builder
)

END_NAMESPACE_OPENDAQ
