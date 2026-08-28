#include <opendaq/credential_payload_descriptor_impl.h>
#include <opendaq/credential_payload_descriptor_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    // Cached once per process - the shapes themselves are defined exactly once, in
    // `credential_payload_descriptor_factory.h` (the single source of truth also used by `Context` to
    // register these types up front).
    static const StructTypePtr keyValuePayloadParametersStructType = KeyValuePayloadDescriptorParametersStructType();
    static const StructTypePtr stringPayloadParametersStructType = StringPayloadDescriptorParametersStructType();
    static const StructTypePtr filePathPayloadParametersStructType = FilePathPayloadDescriptorParametersStructType();

    static const StructTypePtr keyValuePayloadDescriptorStructType = KeyValuePayloadDescriptorStructType();
    static const StructTypePtr stringPayloadDescriptorStructType = StringPayloadDescriptorStructType();
    static const StructTypePtr filePathPayloadDescriptorStructType = FilePathPayloadDescriptorStructType();

    inline const StructTypePtr& LocalParametersStructType(CredentialPayloadFormat format)
    {
        switch (format)
        {
            case CredentialPayloadFormat::KeyValuePairs:
                return keyValuePayloadParametersStructType;
            case CredentialPayloadFormat::String:
                return stringPayloadParametersStructType;
            default:
                return filePathPayloadParametersStructType;
        }
    }

    inline const StructTypePtr& LocalDescriptorStructType(CredentialPayloadFormat format)
    {
        switch (format)
        {
            case CredentialPayloadFormat::KeyValuePairs:
                return keyValuePayloadDescriptorStructType;
            case CredentialPayloadFormat::String:
                return stringPayloadDescriptorStructType;
            default:
                return filePathPayloadDescriptorStructType;
        }
    }

    // If `typeManager` is assigned and already has a type of this name registered (see
    // `RegisterCredentialPayloadDescriptorTypes`), returns that registered type - so the built Struct
    // shares the exact type object the manager knows about, rather than an independently-built one.
    // Otherwise falls back to `localType` (the process-wide cached one above), unregistered.
    inline StructTypePtr ResolveStructType(const StructTypePtr& localType, const TypeManagerPtr& typeManager)
    {
        if (typeManager.assigned() && typeManager.hasType(localType.getName()))
            return typeManager.getType(localType.getName());
        return localType;
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
    // FilePath: no fields at all.

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
    else
        *id = "FilePathPayloadDescriptorParameters";

    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorParametersImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&]
        {
            // No dispatch-key hint is passed to `Deserialize` itself (the same three-keys-one-function
            // pattern as the outer descriptor), so the shape is detected directly from which field is
            // present in the serialized data instead.
            if (serializedObj.hasKey("Keys"))
            {
                const DictPtr<IString, IBoolean> keys = serializedObj.readObject("Keys", contextPtr, factoryCallbackPtr);
                *obj = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
                           detail::LocalParametersStructType(CredentialPayloadFormat::KeyValuePairs), Dict<IString, IBaseObject>({{"Keys", keys}}))
                           .detach();
            }
            else if (serializedObj.hasKey("Hidden"))
            {
                const Bool hidden = serializedObj.readBool("Hidden");
                *obj = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
                           detail::LocalParametersStructType(CredentialPayloadFormat::String), Dict<IString, IBaseObject>({{"Hidden", hidden}}))
                           .detach();
            }
            else
            {
                *obj = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
                           detail::LocalParametersStructType(CredentialPayloadFormat::FilePath), Dict<IString, IBaseObject>())
                           .detach();
            }

            return OPENDAQ_SUCCESS;
        });
}

//
// CredentialPayloadDescriptorImpl
//

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(IString* id, IDict* keys, IString* description, const TypeManagerPtr& typeManager)
{
    const DictPtr<IString, IBoolean> keysPtr = keys;
    if (!keysPtr.assigned() || keysPtr.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value credential payload descriptor");

    const auto parametersType = detail::ResolveStructType(detail::LocalParametersStructType(CredentialPayloadFormat::KeyValuePairs), typeManager);
    const auto parameters =
        createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(parametersType, Dict<IString, IBaseObject>({{"Keys", keysPtr}}));

    return Dict<IString, IBaseObject>({{"Id", StringPtr(id)}, {"Description", StringPtr(description)}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(IString* id, IString* description, Bool hidden, const TypeManagerPtr& typeManager)
{
    const auto parametersType = detail::ResolveStructType(detail::LocalParametersStructType(CredentialPayloadFormat::String), typeManager);
    const auto parameters = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
        parametersType, Dict<IString, IBaseObject>({{"Hidden", hidden}}));

    return Dict<IString, IBaseObject>({{"Id", StringPtr(id)}, {"Description", StringPtr(description)}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(IString* id, IString* description, const TypeManagerPtr& typeManager)
{
    const auto parametersType = detail::ResolveStructType(detail::LocalParametersStructType(CredentialPayloadFormat::FilePath), typeManager);
    const auto parameters =
        createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(parametersType, Dict<IString, IBaseObject>());

    return Dict<IString, IBaseObject>({{"Id", StringPtr(id)}, {"Description", StringPtr(description)}, {"Parameters", parameters}});
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(IString* id, IDict* keys, IString* description, ITypeManager* typeManager)
    : CredentialPayloadDescriptorImpl(CredentialPayloadFormat::KeyValuePairs,
                                      detail::ResolveStructType(detail::LocalDescriptorStructType(CredentialPayloadFormat::KeyValuePairs), typeManager),
                                      BuildFields(id, keys, description, typeManager),
                                      typeManager)
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(IString* id, IString* description, Bool hidden, ITypeManager* typeManager)
    : CredentialPayloadDescriptorImpl(CredentialPayloadFormat::String,
                                      detail::ResolveStructType(detail::LocalDescriptorStructType(CredentialPayloadFormat::String), typeManager),
                                      BuildFields(id, description, hidden, typeManager),
                                      typeManager)
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(IString* id, IString* description, ITypeManager* typeManager)
    : CredentialPayloadDescriptorImpl(CredentialPayloadFormat::FilePath,
                                      detail::ResolveStructType(detail::LocalDescriptorStructType(CredentialPayloadFormat::FilePath), typeManager),
                                      BuildFields(id, description, typeManager),
                                      typeManager)
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(CredentialPayloadFormat format,
                                                                 const StructTypePtr& structType,
                                                                 const DictPtr<IString, IBaseObject>& fields,
                                                                 const TypeManagerPtr& typeManager)
    : GenericStructImpl<ICredentialPayloadDescriptor, IStruct>(structType, fields)
    , format(format)
    , typeManager(typeManager)
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

    if (format == CredentialPayloadFormat::KeyValuePairs)
    {
        // The property set is data-dependent (one per key named in "Parameters"."Keys", which varies by
        // descriptor) - it doesn't fit a single well-known, registered `IPropertyObjectClass`, so it's
        // still built dynamically here.
        auto payloadObj = PropertyObject();
        const StructPtr parameters = this->fields.get("Parameters");
        const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
        for (const auto& [key, hidden] : keys)
            payloadObj.addProperty(StringProperty(key, ""));

        *payload = payloadObj.detach();
        return OPENDAQ_SUCCESS;
    }

    // String and FilePath both need only a single "Secret" property - the exact same shape - so they
    // share one well-known `CredentialSecretPayloadClass`, built from the type manager when it's been
    // registered there (see `RegisterCredentialPayloadDescriptorTypes`).
    if (typeManager.assigned() && typeManager.hasType(CredentialSecretPayloadClassName))
    {
        *payload = PropertyObject(typeManager, CredentialSecretPayloadClassName).detach();
        return OPENDAQ_SUCCESS;
    }

    auto payloadObj = PropertyObject();
    payloadObj.addProperty(StringProperty("Secret", ""));
    *payload = payloadObj.detach();
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

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::getSerializeId(ConstCharPtr* id) const
{
    switch (format)
    {
        case CredentialPayloadFormat::KeyValuePairs:
            *id = "KeyValuePayloadDescriptor";
            break;
        case CredentialPayloadFormat::String:
            *id = "StringPayloadDescriptor";
            break;
        default:
            *id = "FilePathPayloadDescriptor";
            break;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
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
            const TypeManagerPtr typeManagerPtr = contextPtr.asPtrOrNull<ITypeManager>();

            // The nested "Parameters" struct's own type name identifies the format - self-describing, so
            // this doesn't need to know which of the three registered `SerializeId`s dispatched here.
            // Passed as raw interface pointers (matching the constructors' own disambiguation) rather than
            // smart pointers - `id`/`description` are the same `StringPtr` type regardless of format, so a
            // smart-pointer argument here would equally match every overload.
            const StringPtr parametersTypeName = parameters.getStructType().getName();
            if (parametersTypeName == "KeyValuePayloadDescriptorParameters")
            {
                const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(
                           id.getObject(), keys.getObject(), description.getObject(), typeManagerPtr.getObject())
                           .detach();
            }
            else if (parametersTypeName == "StringPayloadDescriptorParameters")
            {
                const Bool hidden = parameters.get("Hidden");
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(
                           id.getObject(), description.getObject(), hidden, typeManagerPtr.getObject())
                           .detach();
            }
            else
            {
                *obj = createWithImplementation<ICredentialPayloadDescriptor, CredentialPayloadDescriptorImpl>(
                           id.getObject(), description.getObject(), typeManagerPtr.getObject())
                           .detach();
            }

            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IDict*, keys, IString*, description, ITypeManager*, typeManager)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringPayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description, Bool, hidden, ITypeManager*, typeManager)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FilePathPayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description, ITypeManager*, typeManager)

namespace detail
{
    // Three registered dispatch keys (matching the three possible `getSerializeId()` results), all backed
    // by the same shared `Deserialize` - it self-describes the format from the nested "Parameters" struct's
    // own type name, so it doesn't need a separate entry point per format.
    class CredentialPayloadDescriptorDeserializeFactory
    {
    public:
        CredentialPayloadDescriptorDeserializeFactory()
        {
            daqRegisterSerializerFactory("KeyValuePayloadDescriptor", CredentialPayloadDescriptorImpl::Deserialize);
            daqRegisterSerializerFactory("StringPayloadDescriptor", CredentialPayloadDescriptorImpl::Deserialize);
            daqRegisterSerializerFactory("FilePathPayloadDescriptor", CredentialPayloadDescriptorImpl::Deserialize);
        }
    };
    static CredentialPayloadDescriptorDeserializeFactory gCredentialPayloadDescriptorDeserializeFactory;

    // Same pattern for the nested "Parameters" struct - three dispatch keys, one shared `Deserialize` that
    // detects the shape from which field ("Keys"/"Hidden"/neither) is present in the serialized data.
    class CredentialPayloadDescriptorParametersDeserializeFactory
    {
    public:
        CredentialPayloadDescriptorParametersDeserializeFactory()
        {
            daqRegisterSerializerFactory("KeyValuePayloadDescriptorParameters", CredentialPayloadDescriptorParametersImpl::Deserialize);
            daqRegisterSerializerFactory("StringPayloadDescriptorParameters", CredentialPayloadDescriptorParametersImpl::Deserialize);
            daqRegisterSerializerFactory("FilePathPayloadDescriptorParameters", CredentialPayloadDescriptorParametersImpl::Deserialize);
        }
    };
    static CredentialPayloadDescriptorParametersDeserializeFactory gCredentialPayloadDescriptorParametersDeserializeFactory;
}

END_NAMESPACE_OPENDAQ
