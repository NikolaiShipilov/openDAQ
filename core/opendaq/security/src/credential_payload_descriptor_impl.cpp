#include <opendaq/credential_payload_descriptor_impl.h>
#include <opendaq/credential_payload_descriptor_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    // Used when reconstructing from serialized data, without a type manager.
    inline StructTypePtr LocalParametersStructType(CredentialPayloadFormat format)
    {
        switch (format)
        {
            case CredentialPayloadFormat::KeyValuePairs:
                return KeyValuePayloadDescriptorParametersStructType();
            case CredentialPayloadFormat::String:
                return StringPayloadDescriptorParametersStructType();
            case CredentialPayloadFormat::FilePath:
                return FilePathPayloadDescriptorParametersStructType();
        }
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown CredentialPayloadFormat value");
    }

    inline StructTypePtr LocalDescriptorStructType(CredentialPayloadFormat format)
    {
        switch (format)
        {
            case CredentialPayloadFormat::KeyValuePairs:
                return KeyValuePayloadDescriptorStructType();
            case CredentialPayloadFormat::String:
                return StringPayloadDescriptorStructType();
            case CredentialPayloadFormat::FilePath:
                return FilePathPayloadDescriptorStructType();
        }

        DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown CredentialPayloadFormat value");
    }

    inline StructTypePtr RequireRegisteredType(const TypeManagerPtr& typeManager, const StringPtr& typeName)
    {
        if (!typeManager.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type manager must be assigned when creating a \"{}\"", typeName);
        if (!typeManager.hasType(typeName))
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type \"{}\" is not registered in the type manager", typeName);
        return typeManager.getType(typeName);
    }

    inline StringPtr RequireRegisteredClassName(const TypeManagerPtr& typeManager, const StringPtr& className)
    {
        if (!typeManager.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type manager must be assigned when creating a credential payload descriptor");
        if (!className.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Payload class name must be assigned");
        if (!typeManager.hasType(className))
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type \"{}\" is not registered in the type manager", className);
        return className;
    }
}

//
// CredentialPayloadDescriptorParametersImpl
//

CredentialPayloadDescriptorParametersImpl::CredentialPayloadDescriptorParametersImpl(const StructTypePtr& structType,
                                                                                     const DictPtr<IString, IBaseObject>& fields)
    : GenericStructImpl<IStruct>(structType, fields)
{
}

ErrCode CredentialPayloadDescriptorParametersImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    const StringPtr typeName = this->structType.getName();
    if (typeName == "KeyValuePayloadDescriptorParameters")
    {
        const DictPtr<IString, IBoolean> keys = this->fields.get("Keys");
        serializer->key("Keys");
        keys.template asPtr<ISerializable>().serialize(serializer);
    }
    else if (typeName == "StringPayloadDescriptorParameters")
    {
        const Bool hidden = this->fields.get("Hidden");
        serializer->key("Hidden");
        serializer->writeBool(hidden);
    }
    else if (typeName == "FilePathPayloadDescriptorParameters")
    {
        // FilePath has no fields.
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Unknown payload descriptor parameters type \"{}\"", typeName.getCharPtr());
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorParametersImpl::getSerializeId(ConstCharPtr* id) const
{
    const StringPtr typeName = this->structType.getName();
    if (typeName == "KeyValuePayloadDescriptorParameters")
        *id = "KeyValuePayloadDescriptorParameters";
    else if (typeName == "StringPayloadDescriptorParameters")
        *id = "StringPayloadDescriptorParameters";
    else if (typeName == "FilePathPayloadDescriptorParameters")
        *id = "FilePathPayloadDescriptorParameters";
    else
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Unknown payload descriptor parameters type \"{}\"", typeName);

    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorParametersImpl::DeserializeKeyValuePairs(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            const DictPtr<IString, IBoolean> keys = serializedObj.readObject("Keys", contextPtr, factoryCallbackPtr);
            *obj = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
                       detail::LocalParametersStructType(CredentialPayloadFormat::KeyValuePairs), Dict<IString, IBaseObject>({{"Keys", keys}}))
                       .detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode CredentialPayloadDescriptorParametersImpl::DeserializeString(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);

    return daqTry(
        [&]
        {
            const Bool hidden = serializedObj.readBool("Hidden");
            *obj = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
                       detail::LocalParametersStructType(CredentialPayloadFormat::String), Dict<IString, IBaseObject>({{"Hidden", hidden}}))
                       .detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode CredentialPayloadDescriptorParametersImpl::DeserializeFilePath(ISerializedObject*, IBaseObject*, IFunction*, IBaseObject** obj)
{
    return daqTry(
        [&]
        {
            *obj = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
                       detail::LocalParametersStructType(CredentialPayloadFormat::FilePath), Dict<IString, IBaseObject>())
                       .detach();
            return OPENDAQ_SUCCESS;
        });
}

//
// CredentialPayloadDescriptorImpl
//

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(IString* id, IDict* keys, IString* description, const StructTypePtr& parametersType)
{
    const DictPtr<IString, IBoolean> keysPtr = keys;
    if (!keysPtr.assigned() || keysPtr.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value credential payload descriptor");

    const auto parameters =
        createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(parametersType, Dict<IString, IBaseObject>({{"Keys", keysPtr}}));

    return Dict<IString, IBaseObject>({{"Id", StringPtr(id)}, {"Description", StringPtr(description)}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(IString* id, IString* description, Bool hidden, const StructTypePtr& parametersType)
{
    const auto parameters = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
        parametersType, Dict<IString, IBaseObject>({{"Hidden", hidden}}));

    return Dict<IString, IBaseObject>({{"Id", StringPtr(id)}, {"Description", StringPtr(description)}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(IString* id, IString* description, const StructTypePtr& parametersType)
{
    const auto parameters =
        createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(parametersType, Dict<IString, IBaseObject>());

    return Dict<IString, IBaseObject>({{"Id", StringPtr(id)}, {"Description", StringPtr(description)}, {"Parameters", parameters}});
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(
    IString* id, IDict* keys, IString* description, ITypeManager* typeManager, IString* payloadClassName)
    : CredentialPayloadDescriptorImpl(
          CredentialPayloadFormat::KeyValuePairs,
          detail::RequireRegisteredType(typeManager, KeyValuePayloadDescriptorStructType().getName()),
          BuildFields(id, keys, description, detail::RequireRegisteredType(typeManager, KeyValuePayloadDescriptorParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, payloadClassName))
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(
    IString* id, IString* description, Bool hidden, ITypeManager* typeManager, IString* payloadClassName)
    : CredentialPayloadDescriptorImpl(
          CredentialPayloadFormat::String,
          detail::RequireRegisteredType(typeManager, StringPayloadDescriptorStructType().getName()),
          BuildFields(id, description, hidden, detail::RequireRegisteredType(typeManager, StringPayloadDescriptorParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, payloadClassName))
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(
    IString* id, IString* description, ITypeManager* typeManager, IString* payloadClassName)
    : CredentialPayloadDescriptorImpl(
          CredentialPayloadFormat::FilePath,
          detail::RequireRegisteredType(typeManager, FilePathPayloadDescriptorStructType().getName()),
          BuildFields(id, description, detail::RequireRegisteredType(typeManager, FilePathPayloadDescriptorParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, payloadClassName))
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(CredentialPayloadFormat format,
                                                                 const StructTypePtr& structType,
                                                                 const DictPtr<IString, IBaseObject>& fields,
                                                                 const TypeManagerPtr& typeManager,
                                                                 const StringPtr& payloadClassName)
    : GenericStructImpl<ICredentialPayloadDescriptor, IStruct>(structType, fields)
    , format(format)
    , typeManager(typeManager)
    , payloadClassName(payloadClassName)
{
}

ErrCode CredentialPayloadDescriptorImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->fields.get("Id").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::getFormat(CredentialPayloadFormat* formatOut)
{
    OPENDAQ_PARAM_NOT_NULL(formatOut);

    *formatOut = this->format;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::getParameters(IStruct** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->fields.get("Parameters").template asPtr<IStruct>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->fields.get("Description").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::createDefaultPayload(IPropertyObject** payload)
{
    OPENDAQ_PARAM_NOT_NULL(payload);

    *payload = PropertyObject(typeManager, payloadClassName).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::serialize(ISerializer* serializer)
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

    serializer->key("PayloadClassName");
    serializer->writeString(payloadClassName.getCharPtr(), payloadClassName.getLength());

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::getSerializeId(ConstCharPtr* id) const
{
    switch (format)
    {
        case CredentialPayloadFormat::KeyValuePairs:
            *id = "KeyValuePayloadDescriptor";
            return OPENDAQ_SUCCESS;
        case CredentialPayloadFormat::String:
            *id = "StringPayloadDescriptor";
            return OPENDAQ_SUCCESS;
        case CredentialPayloadFormat::FilePath:
            *id = "FilePathPayloadDescriptor";
            return OPENDAQ_SUCCESS;
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Unknown CredentialPayloadFormat value");
}

ErrCode CredentialPayloadDescriptorImpl::DeserializeKeyValuePairs(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            const auto id = serializedObj.readString("Id");
            const auto description = serializedObj.readString("Description");
            const StructPtr parameters = serializedObj.readObject("Parameters", contextPtr, factoryCallbackPtr);
            const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
            const auto payloadClassName = serializedObj.readString("PayloadClassName");
            const TypeManagerPtr typeManagerPtr = contextPtr.asPtrOrNull<ITypeManager>();

            const auto fields = BuildFields(id.getObject(), keys.getObject(), description.getObject(),
                                            detail::LocalParametersStructType(CredentialPayloadFormat::KeyValuePairs));
            *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(
                       CredentialPayloadFormat::KeyValuePairs,
                       detail::LocalDescriptorStructType(CredentialPayloadFormat::KeyValuePairs),
                       fields,
                       typeManagerPtr,
                       payloadClassName)
                       .detach();

            return OPENDAQ_SUCCESS;
        });
}

ErrCode CredentialPayloadDescriptorImpl::DeserializeString(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            const auto id = serializedObj.readString("Id");
            const auto description = serializedObj.readString("Description");
            const StructPtr parameters = serializedObj.readObject("Parameters", contextPtr, factoryCallbackPtr);
            const Bool hidden = parameters.get("Hidden");
            const auto payloadClassName = serializedObj.readString("PayloadClassName");

            const TypeManagerPtr typeManagerPtr = contextPtr.asPtrOrNull<ITypeManager>();

            const auto fields = BuildFields(id.getObject(), description.getObject(), hidden,
                                            detail::LocalParametersStructType(CredentialPayloadFormat::String));
            *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(
                       CredentialPayloadFormat::String,
                       detail::LocalDescriptorStructType(CredentialPayloadFormat::String),
                       fields,
                       typeManagerPtr,
                       payloadClassName)
                       .detach();

            return OPENDAQ_SUCCESS;
        });
}

ErrCode CredentialPayloadDescriptorImpl::DeserializeFilePath(ISerializedObject* serialized, IBaseObject* context, IFunction*, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);

    return daqTry(
        [&]
        {
            const auto id = serializedObj.readString("Id");
            const auto description = serializedObj.readString("Description");
            const auto payloadClassName = serializedObj.readString("PayloadClassName");

            const TypeManagerPtr typeManagerPtr = contextPtr.asPtrOrNull<ITypeManager>();

            const auto fields = BuildFields(id.getObject(), description.getObject(),
                                            detail::LocalParametersStructType(CredentialPayloadFormat::FilePath));
            *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(
                       CredentialPayloadFormat::FilePath,
                       detail::LocalDescriptorStructType(CredentialPayloadFormat::FilePath),
                       fields,
                       typeManagerPtr,
                       payloadClassName)
                       .detach();

            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IDict*, keys, IString*, description, ITypeManager*, typeManager, IString*, payloadClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringPayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description, Bool, hidden, ITypeManager*, typeManager, IString*, payloadClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FilePathPayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description, ITypeManager*, typeManager, IString*, payloadClassName)

namespace detail
{
    class CredentialPayloadDescriptorDeserializeFactory
    {
    public:
        CredentialPayloadDescriptorDeserializeFactory()
        {
            daqRegisterSerializerFactory("KeyValuePayloadDescriptor", CredentialPayloadDescriptorImpl::DeserializeKeyValuePairs);
            daqRegisterSerializerFactory("StringPayloadDescriptor", CredentialPayloadDescriptorImpl::DeserializeString);
            daqRegisterSerializerFactory("FilePathPayloadDescriptor", CredentialPayloadDescriptorImpl::DeserializeFilePath);
        }
    };
    static CredentialPayloadDescriptorDeserializeFactory gCredentialPayloadDescriptorDeserializeFactory;

    // Same pattern for the nested "Parameters" struct.
    class CredentialPayloadDescriptorParametersDeserializeFactory
    {
    public:
        CredentialPayloadDescriptorParametersDeserializeFactory()
        {
            daqRegisterSerializerFactory("KeyValuePayloadDescriptorParameters", CredentialPayloadDescriptorParametersImpl::DeserializeKeyValuePairs);
            daqRegisterSerializerFactory("StringPayloadDescriptorParameters", CredentialPayloadDescriptorParametersImpl::DeserializeString);
            daqRegisterSerializerFactory("FilePathPayloadDescriptorParameters", CredentialPayloadDescriptorParametersImpl::DeserializeFilePath);
        }
    };
    static CredentialPayloadDescriptorParametersDeserializeFactory gCredentialPayloadDescriptorParametersDeserializeFactory;
}

END_NAMESPACE_OPENDAQ
