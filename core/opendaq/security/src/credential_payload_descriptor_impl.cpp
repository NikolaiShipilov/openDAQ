#include <opendaq/credential_payload_descriptor_impl.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    inline StructTypePtr MakeKeyValuePayloadParametersStructType()
    {
        return StructType("KeyValuePayloadDescriptorParameters", List<IString>("Keys"), List<IType>(SimpleType(ctDict)));
    }

    inline StructTypePtr MakeStringPayloadParametersStructType()
    {
        return StructType("StringPayloadDescriptorParameters", List<IString>("Hidden"), List<IType>(SimpleType(ctBool)));
    }

    inline StructTypePtr MakeFilePathPayloadParametersStructType()
    {
        return StructType("FilePathPayloadDescriptorParameters", List<IString>(), List<IType>());
    }

    static const StructTypePtr keyValuePayloadParametersStructType = MakeKeyValuePayloadParametersStructType();
    static const StructTypePtr stringPayloadParametersStructType = MakeStringPayloadParametersStructType();
    static const StructTypePtr filePathPayloadParametersStructType = MakeFilePathPayloadParametersStructType();

    template <CredentialPayloadFormat Format>
    const StructTypePtr& PayloadParametersStructType()
    {
        if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
            return keyValuePayloadParametersStructType;
        else if constexpr (Format == CredentialPayloadFormat::String)
            return stringPayloadParametersStructType;
        else
            return filePathPayloadParametersStructType;
    }

    inline StructTypePtr MakeKeyValuePayloadDescriptorStructType()
    {
        return StructType("KeyValuePayloadDescriptor",
                          List<IString>("Id", "Description", "Parameters"),
                          List<IType>(SimpleType(ctString), SimpleType(ctString), keyValuePayloadParametersStructType));
    }

    inline StructTypePtr MakeStringPayloadDescriptorStructType()
    {
        return StructType("StringPayloadDescriptor",
                          List<IString>("Id", "Description", "Parameters"),
                          List<IType>(SimpleType(ctString), SimpleType(ctString), stringPayloadParametersStructType));
    }

    inline StructTypePtr MakeFilePathPayloadDescriptorStructType()
    {
        return StructType("FilePathPayloadDescriptor",
                          List<IString>("Id", "Description", "Parameters"),
                          List<IType>(SimpleType(ctString), SimpleType(ctString), filePathPayloadParametersStructType));
    }

    static const StructTypePtr keyValuePayloadDescriptorStructType = MakeKeyValuePayloadDescriptorStructType();
    static const StructTypePtr stringPayloadDescriptorStructType = MakeStringPayloadDescriptorStructType();
    static const StructTypePtr filePathPayloadDescriptorStructType = MakeFilePathPayloadDescriptorStructType();

    template <CredentialPayloadFormat Format>
    const StructTypePtr& PayloadDescriptorStructType()
    {
        if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
            return keyValuePayloadDescriptorStructType;
        else if constexpr (Format == CredentialPayloadFormat::String)
            return stringPayloadDescriptorStructType;
        else
            return filePathPayloadDescriptorStructType;
    }
}

//
// CredentialPayloadParametersImpl
//

template <CredentialPayloadFormat Format>
CredentialPayloadParametersImpl<Format>::CredentialPayloadParametersImpl(const DictPtr<IString, IBoolean>& keys)
    : GenericStructImpl<IStruct>(detail::PayloadParametersStructType<Format>(), Dict<IString, IBaseObject>({{"Keys", keys}}))
{
}

template <CredentialPayloadFormat Format>
CredentialPayloadParametersImpl<Format>::CredentialPayloadParametersImpl(Bool hidden)
    : GenericStructImpl<IStruct>(detail::PayloadParametersStructType<Format>(), Dict<IString, IBaseObject>({{"Hidden", hidden}}))
{
}

template <CredentialPayloadFormat Format>
CredentialPayloadParametersImpl<Format>::CredentialPayloadParametersImpl()
    : GenericStructImpl<IStruct>(detail::PayloadParametersStructType<Format>(), Dict<IString, IBaseObject>())
{
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadParametersImpl<Format>::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
    {
        const DictPtr<IString, IBoolean> keys = this->fields.get("Keys");
        serializer->key("Keys");
        keys.template asPtr<ISerializable>().serialize(serializer);
    }
    else if constexpr (Format == CredentialPayloadFormat::String)
    {
        const Bool hidden = this->fields.get("Hidden");
        serializer->key("Hidden");
        serializer->writeBool(hidden);
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadParametersImpl<Format>::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ConstCharPtr CredentialPayloadParametersImpl<Format>::SerializeId()
{
    if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
        return "KeyValuePayloadDescriptorParameters";
    else if constexpr (Format == CredentialPayloadFormat::String)
        return "StringPayloadDescriptorParameters";
    else
        return "FilePathPayloadDescriptorParameters";
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadParametersImpl<Format>::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
            {
                const DictPtr<IString, IBoolean> keys = serializedObj.readObject("Keys", contextPtr, factoryCallbackPtr);
                *obj = createWithImplementation<IStruct, CredentialPayloadParametersImpl>(keys).detach();
            }
            else if constexpr (Format == CredentialPayloadFormat::String)
            {
                const Bool hidden = serializedObj.readBool("Hidden");
                *obj = createWithImplementation<IStruct, CredentialPayloadParametersImpl>(hidden).detach();
            }
            else
            {
                *obj = createWithImplementation<IStruct, CredentialPayloadParametersImpl>().detach();
            }

            return OPENDAQ_SUCCESS;
        });
}

//
// CredentialPayloadDescriptorImpl
//

template <CredentialPayloadFormat Format>
DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl<Format>::BuildFields(const StringPtr& id, const DictPtr<IString, IBoolean>& keys, const StringPtr& description)
{
    if (!keys.assigned() || keys.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value credential payload descriptor");

    return Dict<IString, IBaseObject>(
        {{"Id", id}, {"Description", description}, {"Parameters", createWithImplementation<IStruct, CredentialPayloadParametersImpl<Format>>(keys)}});
}

template <CredentialPayloadFormat Format>
DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl<Format>::BuildFields(const StringPtr& id, const StringPtr& description, Bool hidden)
{
    return Dict<IString, IBaseObject>(
        {{"Id", id}, {"Description", description}, {"Parameters", createWithImplementation<IStruct, CredentialPayloadParametersImpl<Format>>(hidden)}});
}

template <CredentialPayloadFormat Format>
DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl<Format>::BuildFields(const StringPtr& id, const StringPtr& description)
{
    return Dict<IString, IBaseObject>(
        {{"Id", id}, {"Description", description}, {"Parameters", createWithImplementation<IStruct, CredentialPayloadParametersImpl<Format>>()}});
}

template <CredentialPayloadFormat Format>
CredentialPayloadDescriptorImpl<Format>::CredentialPayloadDescriptorImpl(const StringPtr& id, const DictPtr<IString, IBoolean>& keys, const StringPtr& description)
    : GenericStructImpl<ICredentialPayloadDescriptor, IStruct>(detail::PayloadDescriptorStructType<Format>(), BuildFields(id, keys, description))
{
}

template <CredentialPayloadFormat Format>
CredentialPayloadDescriptorImpl<Format>::CredentialPayloadDescriptorImpl(const StringPtr& id, const StringPtr& description, Bool hidden)
    : GenericStructImpl<ICredentialPayloadDescriptor, IStruct>(detail::PayloadDescriptorStructType<Format>(), BuildFields(id, description, hidden))
{
}

template <CredentialPayloadFormat Format>
CredentialPayloadDescriptorImpl<Format>::CredentialPayloadDescriptorImpl(const StringPtr& id, const StringPtr& description)
    : GenericStructImpl<ICredentialPayloadDescriptor, IStruct>(detail::PayloadDescriptorStructType<Format>(), BuildFields(id, description))
{
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->fields.get("Id").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::getFormat(CredentialPayloadFormat* format)
{
    OPENDAQ_PARAM_NOT_NULL(format);

    *format = Format;
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::getParameters(IStruct** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->fields.get("Parameters").template asPtr<IStruct>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->fields.get("Description").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::createDefaultPayload(IPropertyObject** payload)
{
    OPENDAQ_PARAM_NOT_NULL(payload);

    auto payloadObj = PropertyObject();

    if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
    {
        const StructPtr parameters = this->fields.get("Parameters");
        const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
        for (const auto& [key, hidden] : keys)
            payloadObj.addProperty(StringProperty(key, ""));
    }
    else
    {
        payloadObj.addProperty(StringProperty("Secret", ""));
    }

    *payload = payloadObj.detach();
    return OPENDAQ_SUCCESS;
}

template <CredentialPayloadFormat Format>
ErrCode CredentialPayloadDescriptorImpl<Format>::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    const StringPtr id = this->fields.get("Id");
    serializer->key("Id");
    serializer->writeString(id.getCharPtr(), id.getLength());

    const StringPtr description = this->fields.get("Description");
    serializer->key("Description");
    serializer->writeString(description.getCharPtr(), description.getLength());

    const StructPtr parameters = this->fields.get("Parameters");
    serializer->key("Parameters");
    parameters.template asPtr<ISerializable>().serialize(serializer);

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
    if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
        return "KeyValuePayloadDescriptor";
    else if constexpr (Format == CredentialPayloadFormat::String)
        return "StringPayloadDescriptor";
    else if constexpr (Format == CredentialPayloadFormat::FilePath)
        return "FilePathPayloadDescriptor";
    else
        return "";
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
            const auto id = serializedObj.readString("Id");
            const auto description = serializedObj.readString("Description");

            if constexpr (Format == CredentialPayloadFormat::KeyValuePairs)
            {
                const StructPtr parameters = serializedObj.readObject("Parameters", contextPtr, factoryCallbackPtr);
                const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(id, keys, description).detach();
            }
            else if constexpr (Format == CredentialPayloadFormat::String)
            {
                const StructPtr parameters = serializedObj.readObject("Parameters", contextPtr, factoryCallbackPtr);
                const Bool hidden = parameters.get("Hidden");
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(id, description, hidden).detach();
            }
            else
            {
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(id, description).detach();
            }

            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IDict*, keys, IString*, description)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringPayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description, Bool, hidden)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FilePathPayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description)

END_NAMESPACE_OPENDAQ
