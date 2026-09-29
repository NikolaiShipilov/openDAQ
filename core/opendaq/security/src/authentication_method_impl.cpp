#include <opendaq/authentication_method_impl.h>
#include <opendaq/authentication_method_factory.h>
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
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type manager must be assigned when creating an authentication method");
        if (!className.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Secret class name must be assigned");
        if (!typeManager.hasType(className))
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type \"{}\" is not registered in the type manager", className);
        return className;
    }
}

//
// AuthenticationMethodParametersImpl
//

AuthenticationMethodParametersImpl::AuthenticationMethodParametersImpl(const StructTypePtr& structType,
                                                                       const DictPtr<IString, IBaseObject>& fields)
    : GenericStructImpl<IStruct>(structType, fields)
{
}

//
// AuthenticationMethodImpl
//

DictPtr<IString, IBaseObject> AuthenticationMethodImpl::BuildFields(
    const StringPtr& id,
    const DictPtr<IString, IBoolean>& keys,
    const StringPtr& description,
    const StructTypePtr& parametersType)
{
    if (!keys.assigned() || keys.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value authentication method");

    const auto parameters =
        createWithImplementation<IStruct, AuthenticationMethodParametersImpl>(parametersType, Dict<IString, IBaseObject>({{"Keys", keys}}));

    return Dict<IString, IBaseObject>({{"AuthenticationMethodId", id}, {"Description", description}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> AuthenticationMethodImpl::BuildFields(
    const StringPtr& id,
    const StringPtr& description,
    Bool hidden,
    const StructTypePtr& parametersType)
{
    const auto parameters = createWithImplementation<IStruct, AuthenticationMethodParametersImpl>(
        parametersType, Dict<IString, IBaseObject>({{"Hidden", hidden}}));

    return Dict<IString, IBaseObject>({{"AuthenticationMethodId", id}, {"Description", description}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> AuthenticationMethodImpl::BuildFields(const StringPtr& id, const StringPtr& description)
{
    return Dict<IString, IBaseObject>({{"AuthenticationMethodId", id}, {"Description", description}});
}

AuthenticationMethodImpl::AuthenticationMethodImpl(
    const StringPtr& id,
    const DictPtr<IString, IBoolean>& keys,
    const StringPtr& description,
    const TypeManagerPtr& typeManager,
    const StringPtr& secretClassName)
    : AuthenticationMethodImpl(
          CredentialFormat::KeyValuePairs,
          detail::RequireRegisteredType(typeManager, KeyValueAuthenticationMethodStructType().getName()),
          BuildFields(id, keys, description, detail::RequireRegisteredType(typeManager, KeyValueAuthenticationMethodParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, secretClassName))
{
}

AuthenticationMethodImpl::AuthenticationMethodImpl(
    const StringPtr& id, const StringPtr& description, Bool hidden, const TypeManagerPtr& typeManager, const StringPtr& secretClassName)
    : AuthenticationMethodImpl(
          CredentialFormat::String,
          detail::RequireRegisteredType(typeManager, StringAuthenticationMethodStructType().getName()),
          BuildFields(id, description, hidden, detail::RequireRegisteredType(typeManager, StringAuthenticationMethodParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, secretClassName))
{
}

AuthenticationMethodImpl::AuthenticationMethodImpl(
    const StringPtr& id, const StringPtr& description, const TypeManagerPtr& typeManager, const StringPtr& secretClassName)
    : AuthenticationMethodImpl(
          CredentialFormat::FilePath,
          detail::RequireRegisteredType(typeManager, FilePathAuthenticationMethodStructType().getName()),
          BuildFields(id, description),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, secretClassName))
{
}

AuthenticationMethodImpl::AuthenticationMethodImpl(const StringPtr& id, const StringPtr& description)
    : AuthenticationMethodImpl(CredentialFormat::None, NoneAuthenticationMethodStructType(), BuildFields(id, description), nullptr, nullptr)
{
}

AuthenticationMethodImpl::AuthenticationMethodImpl(CredentialFormat format,
                                                    const StructTypePtr& structType,
                                                    const DictPtr<IString, IBaseObject>& fields,
                                                    const TypeManagerPtr& typeManager,
                                                    const StringPtr& secretClassName)
    : GenericStructImpl<IAuthenticationMethod, IStruct>(structType, fields)
    , format(format)
    , typeManager(typeManager)
    , secretClassName(secretClassName)
{
}

ErrCode AuthenticationMethodImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->fields.get("AuthenticationMethodId").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::getFormat(CredentialFormat* formatOut)
{
    OPENDAQ_PARAM_NOT_NULL(formatOut);

    *formatOut = this->format;
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::getParameters(IStruct** parameters)
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

ErrCode AuthenticationMethodImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->fields.get("Description").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::createEmptySecret(IPropertyObject** secret)
{
    OPENDAQ_PARAM_NOT_NULL(secret);

    if (format == CredentialFormat::None)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The \"None\" format requires no credentials, so it has no secret to build");

    *secret = PropertyObject(typeManager, secretClassName).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::serialize(ISerializer* serializer)
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

ErrCode AuthenticationMethodImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr AuthenticationMethodImpl::SerializeId()
{
    return "AuthenticationMethod";
}

ErrCode AuthenticationMethodImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
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

        AuthenticationMethodPtr result;
        if (typeName == KeyValueAuthenticationMethodStructType().getName())
        {
            const StructPtr parameters = fields.get("Parameters");
            const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
            result = KeyValueAuthenticationMethod(id, keys, description, typeManager, secretClassName);
        }
        else if (typeName == StringAuthenticationMethodStructType().getName())
        {
            const StructPtr parameters = fields.get("Parameters");
            const Bool hidden = parameters.get("Hidden");
            result = StringAuthenticationMethod(id, description, hidden, typeManager, secretClassName);
        }
        else if (typeName == FilePathAuthenticationMethodStructType().getName())
        {
            result = FilePathAuthenticationMethod(id, description, typeManager, secretClassName);
        }
        else if (typeName == NoneAuthenticationMethodStructType().getName())
        {
            result = NoneAuthenticationMethod(id, description);
        }
        else
        {
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown authentication method type \"{}\"", typeName);
        }

        *obj = result.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValueAuthenticationMethod, IAuthenticationMethod, IString*, id, IDict*, keys, IString*, description, ITypeManager*, typeManager, IString*, secretClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringAuthenticationMethod, IAuthenticationMethod, IString*, id, IString*, description, Bool, hidden, ITypeManager*, typeManager, IString*, secretClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FilePathAuthenticationMethod, IAuthenticationMethod, IString*, id, IString*, description, ITypeManager*, typeManager, IString*, secretClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NoneAuthenticationMethod, IAuthenticationMethod, IString*, id, IString*, description)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationMethodImpl)

END_NAMESPACE_OPENDAQ
