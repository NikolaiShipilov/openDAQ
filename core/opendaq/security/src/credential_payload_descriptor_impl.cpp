#include <opendaq/credential_payload_descriptor_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <CredentialPayloadFormat Format>
CredentialPayloadDescriptorImpl<Format>::CredentialPayloadDescriptorImpl(const DictPtr<IString, IBoolean>& keys, const StringPtr& description)
    : description(description)
{
    if (!keys.assigned() || keys.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value credential payload descriptor");

    parameters = PropertyObject();
    parameters.addProperty(DictProperty("Keys", keys));
}

template <CredentialPayloadFormat Format>
CredentialPayloadDescriptorImpl<Format>::CredentialPayloadDescriptorImpl(const StringPtr& description, Bool hidden)
    : description(description)
{
    parameters = PropertyObject();
    parameters.addProperty(BoolProperty("Hidden", hidden));
}

template <CredentialPayloadFormat Format>
CredentialPayloadDescriptorImpl<Format>::CredentialPayloadDescriptorImpl(const StringPtr& description)
    : parameters(PropertyObject())
    , description(description)
{
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::getFormat(CredentialPayloadFormat* format)
{
    OPENDAQ_PARAM_NOT_NULL(format);

    *format = Format;
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::getParameters(IPropertyObject** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->parameters.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    serializer->key("Description");
    serializer->writeString(description.getCharPtr(), description.getLength());

    serializer->key("Parameters");
    parameters.asPtr<ISerializable>().serialize(serializer);

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ConstCharPtr CredentialPayloadDescriptorImpl<Format>::SerializeId()
{
    switch (Format)
    {
        case CredentialPayloadFormat::KeyValuePairs:
            return "KeyValuePayloadDescriptor";
        case CredentialPayloadFormat::String:
            return "StringPayloadDescriptor";
        case CredentialPayloadFormat::FilePath:
            return "FilePathPayloadDescriptor";
        case CredentialPayloadFormat::BinaryBlob:
            return "BinaryBlobPayloadDescriptor";
        default:
            return "";
    }
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            const auto description = serializedObj.readString("Description");

            if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
            {
                const PropertyObjectPtr parameters = serializedObj.readObject("Parameters", contextPtr, factoryCallbackPtr);
                const DictPtr<IString, IBoolean> keys = parameters.getPropertyValue("Keys");
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(keys, description).detach();
            }
            else if constexpr (Format == CredentialPayloadFormat::String)
            {
                const PropertyObjectPtr parameters = serializedObj.readObject("Parameters", contextPtr, factoryCallbackPtr);
                const Bool hidden = parameters.getPropertyValue("Hidden");
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(description, hidden).detach();
            }
            else
            {
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(description).detach();
            }

            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor, IDict*, keys, IString*, description)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringPayloadDescriptor, ICredentialPayloadDescriptor, IString*, description, Bool, hidden)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FilePathPayloadDescriptor, ICredentialPayloadDescriptor, IString*, description)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, BinaryBlobPayloadDescriptor, ICredentialPayloadDescriptor, IString*, description)

END_NAMESPACE_OPENDAQ
