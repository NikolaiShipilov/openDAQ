#include <opendaq/credential_descriptor_impl.h>
#include <opendaq/credential_descriptor_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/ctutils.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
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
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type manager must be assigned when creating a credential descriptor");
        if (!className.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Secret class name must be assigned");
        if (!typeManager.hasType(className))
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type \"{}\" is not registered in the type manager", className);
        return className;
    }
}

//
// CredentialDescriptorParametersImpl
//

CredentialDescriptorParametersImpl::CredentialDescriptorParametersImpl(const StructTypePtr& structType,
                                                                       const DictPtr<IString, IBaseObject>& fields)
    : GenericStructImpl<IStruct>(structType, fields)
{
}

//
// CredentialDescriptorImpl
//

DictPtr<IString, IBaseObject> CredentialDescriptorImpl::BuildFields(
    const StringPtr& id,
    const DictPtr<IString, IBoolean>& keys,
    const StringPtr& description,
    const StructTypePtr& parametersType)
{
    if (!keys.assigned() || keys.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value credential descriptor");

    const auto parameters =
        createWithImplementation<IStruct, CredentialDescriptorParametersImpl>(parametersType, Dict<IString, IBaseObject>({{"Keys", keys}}));

    return Dict<IString, IBaseObject>({{"AuthenticationMethodId", id}, {"Description", description}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> CredentialDescriptorImpl::BuildFields(
    const StringPtr& id,
    const StringPtr& description,
    Bool hidden,
    const StructTypePtr& parametersType)
{
    const auto parameters = createWithImplementation<IStruct, CredentialDescriptorParametersImpl>(
        parametersType, Dict<IString, IBaseObject>({{"Hidden", hidden}}));

    return Dict<IString, IBaseObject>({{"AuthenticationMethodId", id}, {"Description", description}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> CredentialDescriptorImpl::BuildFields(const StringPtr& id, const StringPtr& description)
{
    return Dict<IString, IBaseObject>({{"AuthenticationMethodId", id}, {"Description", description}});
}

CredentialDescriptorImpl::CredentialDescriptorImpl(
    const StringPtr& id,
    const DictPtr<IString, IBoolean>& keys,
    const StringPtr& description,
    const TypeManagerPtr& typeManager,
    const StringPtr& secretClassName)
    : CredentialDescriptorImpl(
          CredentialFormat::KeyValuePairs,
          detail::RequireRegisteredType(typeManager, KeyValueDescriptorStructType().getName()),
          BuildFields(id, keys, description, detail::RequireRegisteredType(typeManager, KeyValueDescriptorParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, secretClassName))
{
}

CredentialDescriptorImpl::CredentialDescriptorImpl(
    const StringPtr& id, const StringPtr& description, Bool hidden, const TypeManagerPtr& typeManager, const StringPtr& secretClassName)
    : CredentialDescriptorImpl(
          CredentialFormat::String,
          detail::RequireRegisteredType(typeManager, StringDescriptorStructType().getName()),
          BuildFields(id, description, hidden, detail::RequireRegisteredType(typeManager, StringDescriptorParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, secretClassName))
{
}

CredentialDescriptorImpl::CredentialDescriptorImpl(
    const StringPtr& id, const StringPtr& description, const TypeManagerPtr& typeManager, const StringPtr& secretClassName)
    : CredentialDescriptorImpl(
          CredentialFormat::FilePath,
          detail::RequireRegisteredType(typeManager, FilePathDescriptorStructType().getName()),
          BuildFields(id, description),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, secretClassName))
{
}

CredentialDescriptorImpl::CredentialDescriptorImpl(const StringPtr& id, const StringPtr& description)
    : CredentialDescriptorImpl(CredentialFormat::None, NoneDescriptorStructType(), BuildFields(id, description), nullptr, nullptr)
{
}

CredentialDescriptorImpl::CredentialDescriptorImpl(CredentialFormat format,
                                                    const StructTypePtr& structType,
                                                    const DictPtr<IString, IBaseObject>& fields,
                                                    const TypeManagerPtr& typeManager,
                                                    const StringPtr& secretClassName)
    : GenericStructImpl<ICredentialDescriptor, IStruct>(structType, fields)
    , format(format)
    , typeManager(typeManager)
    , secretClassName(secretClassName)
{
}

ErrCode CredentialDescriptorImpl::getAuthenticationMethodId(IString** authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    *authenticationMethodId = this->fields.get("AuthenticationMethodId").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialDescriptorImpl::getFormat(CredentialFormat* formatOut)
{
    OPENDAQ_PARAM_NOT_NULL(formatOut);

    *formatOut = this->format;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialDescriptorImpl::getParameters(IStruct** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    if (!this->fields.hasKey("Parameters"))
    {
        *parameters = nullptr;
        return OPENDAQ_SUCCESS;
    }

    *parameters = this->fields.get("Parameters").template asPtr<IStruct>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialDescriptorImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->fields.get("Description").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialDescriptorImpl::createEmptySecret(IPropertyObject** secret)
{
    OPENDAQ_PARAM_NOT_NULL(secret);

    if (format == CredentialFormat::None)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The \"None\" format requires no credentials, so it has no secret to build");

    *secret = PropertyObject(typeManager, secretClassName).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialDescriptorImpl::serialize(ISerializer* serializer)
{
    return daqTry([&]
    {
        serializer->startTaggedObject(this);

        const StringPtr typeName = this->structType.getName();
        serializer->key("typeName");
        serializer->writeString(typeName.getCharPtr(), typeName.getLength());

        serializer->key("fields");
        const auto serializableFields = this->fields.asPtr<ISerializable>(true);
        checkErrorInfo(serializableFields->serialize(serializer));

        if (secretClassName.assigned())
        {
            serializer->key(SecretClassNameSerializedKey);
            serializer->writeString(secretClassName.getCharPtr(), secretClassName.getLength());
        }

        serializer->endObject();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode CredentialDescriptorImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr CredentialDescriptorImpl::SerializeId()
{
    return "CredentialDescriptor";
}

ErrCode CredentialDescriptorImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto typeManager = BaseObjectPtr::Borrow(context).asPtr<ITypeManager>(true);

        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
        const StringPtr typeName = serializedObj.readString("typeName");
        const DictPtr<IString, IBaseObject> fields = serializedObj.readObject("fields", context, factoryCallback).asPtr<IDict>();

        StringPtr secretClassName;
        if (serializedObj.hasKey(SecretClassNameSerializedKey))
            secretClassName = serializedObj.readString(SecretClassNameSerializedKey);

        const StringPtr id = fields.get("AuthenticationMethodId");
        const StringPtr description = fields.get("Description");

        CredentialDescriptorPtr result;
        if (typeName == KeyValueDescriptorStructType().getName())
        {
            const StructPtr parameters = fields.get("Parameters");
            const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
            result = KeyValueDescriptor(id, keys, description, typeManager, secretClassName);
        }
        else if (typeName == StringDescriptorStructType().getName())
        {
            const StructPtr parameters = fields.get("Parameters");
            const Bool hidden = parameters.get("Hidden");
            result = StringDescriptor(id, description, hidden, typeManager, secretClassName);
        }
        else if (typeName == FilePathDescriptorStructType().getName())
        {
            result = FilePathDescriptor(id, description, typeManager, secretClassName);
        }
        else if (typeName == NoneDescriptorStructType().getName())
        {
            result = NoneDescriptor(id, description);
        }
        else
        {
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown credential descriptor type \"{}\"", typeName);
        }

        *obj = result.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValueDescriptor, ICredentialDescriptor, IString*, id, IDict*, keys, IString*, description, ITypeManager*, typeManager, IString*, secretClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringDescriptor, ICredentialDescriptor, IString*, id, IString*, description, Bool, hidden, ITypeManager*, typeManager, IString*, secretClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FilePathDescriptor, ICredentialDescriptor, IString*, id, IString*, description, ITypeManager*, typeManager, IString*, secretClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NoneDescriptor, ICredentialDescriptor, IString*, id, IString*, description)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(CredentialDescriptorImpl)

END_NAMESPACE_OPENDAQ
