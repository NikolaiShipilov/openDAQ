#include <opendaq/credential_payload_descriptor_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

CredentialPayloadDescriptorBaseImpl::CredentialPayloadDescriptorBaseImpl(const PropertyObjectPtr& parameters, const StringPtr& description)
    : parameters(parameters)
    , description(description)
{
}

ErrCode CredentialPayloadDescriptorBaseImpl::getParameters(IPropertyObject** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->parameters.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorBaseImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorBaseImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    serializeCustomValues(serializer);
    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

void CredentialPayloadDescriptorBaseImpl::serializeCustomValues(ISerializer* serializer)
{
    serializer->key("Description");
    serializer->writeString(description.getCharPtr(), description.getLength());
}

KeyValuePayloadDescriptorImpl::KeyValuePayloadDescriptorImpl(const ListPtr<IString>& keys, const StringPtr& description)
    : CredentialPayloadDescriptorBaseImpl(BuildParameters(keys), description)
    , keys(keys)
{
}

PropertyObjectPtr KeyValuePayloadDescriptorImpl::BuildParameters(const ListPtr<IString>& keys)
{
    if (!keys.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned when creating a key-value credential payload descriptor");

    auto params = PropertyObject();
    params.addProperty(ListProperty("Keys", keys));
    return params;
}

ErrCode KeyValuePayloadDescriptorImpl::getFormat(CredentialPayloadFormat* format)
{
    OPENDAQ_PARAM_NOT_NULL(format);

    *format = CredentialPayloadFormat::KeyValuePairs;
    return OPENDAQ_SUCCESS;
}

ErrCode KeyValuePayloadDescriptorImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr KeyValuePayloadDescriptorImpl::SerializeId()
{
    return "KeyValuePayloadDescriptor";
}

void KeyValuePayloadDescriptorImpl::serializeCustomValues(ISerializer* serializer)
{
    CredentialPayloadDescriptorBaseImpl::serializeCustomValues(serializer);

    serializer->key("Keys");
    serializer->startList();
    for (const auto& key : keys)
        serializer->writeString(key.getCharPtr(), key.getLength());
    serializer->endList();
}

ErrCode KeyValuePayloadDescriptorImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            const auto keys = serializedObj.readList<IString>("Keys", contextPtr, factoryCallbackPtr);
            const auto description = serializedObj.readString("Description");

            *obj = createWithImplementation<ICredentialPayloadDescriptor, KeyValuePayloadDescriptorImpl>(keys, description).detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor, IList*, keys, IString*, description)

END_NAMESPACE_OPENDAQ
