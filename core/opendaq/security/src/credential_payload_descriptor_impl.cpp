#include <opendaq/credential_payload_descriptor_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/dictobject_factory.h>

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

    serializer->key("Parameters");
    parameters.asPtr<ISerializable>().serialize(serializer);
}

KeyValuePayloadDescriptorImpl::KeyValuePayloadDescriptorImpl(const DictPtr<IString, IBoolean>& keys, const StringPtr& description)
    : CredentialPayloadDescriptorBaseImpl(BuildParameters(keys), description)
{
}

PropertyObjectPtr KeyValuePayloadDescriptorImpl::BuildParameters(const DictPtr<IString, IBoolean>& keys)
{
    if (!keys.assigned() || keys.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value credential payload descriptor");

    auto params = PropertyObject();
    params.addProperty(DictProperty("Keys", keys));
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

ErrCode KeyValuePayloadDescriptorImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            const PropertyObjectPtr parameters = serializedObj.readObject("Parameters", contextPtr, factoryCallbackPtr);
            const DictPtr<IString, IBoolean> keys = parameters.getPropertyValue("Keys");
            const auto description = serializedObj.readString("Description");

            *obj = createWithImplementation<ICredentialPayloadDescriptor, KeyValuePayloadDescriptorImpl>(keys, description).detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor, IDict*, keys, IString*, description)

StringPayloadDescriptorImpl::StringPayloadDescriptorImpl(const StringPtr& description, Bool hidden)
    : CredentialPayloadDescriptorBaseImpl(BuildParameters(hidden), description)
{
}

PropertyObjectPtr StringPayloadDescriptorImpl::BuildParameters(Bool hidden)
{
    auto params = PropertyObject();
    params.addProperty(BoolProperty("Hidden", hidden));
    return params;
}

ErrCode StringPayloadDescriptorImpl::getFormat(CredentialPayloadFormat* format)
{
    OPENDAQ_PARAM_NOT_NULL(format);

    *format = CredentialPayloadFormat::String;
    return OPENDAQ_SUCCESS;
}

ErrCode StringPayloadDescriptorImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr StringPayloadDescriptorImpl::SerializeId()
{
    return "StringPayloadDescriptor";
}

ErrCode StringPayloadDescriptorImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            const PropertyObjectPtr parameters = serializedObj.readObject("Parameters", contextPtr, factoryCallbackPtr);
            const Bool hidden = parameters.getPropertyValue("Hidden");
            const auto description = serializedObj.readString("Description");

            *obj = createWithImplementation<ICredentialPayloadDescriptor, StringPayloadDescriptorImpl>(description, hidden).detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringPayloadDescriptor, ICredentialPayloadDescriptor, IString*, description, Bool, hidden)

END_NAMESPACE_OPENDAQ
