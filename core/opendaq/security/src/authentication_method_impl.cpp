#include <opendaq/authentication_method_impl.h>
#include <opendaq/authentication_method_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/serializer_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/ctutils.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    inline ConstCharPtr FormatToString(CredentialFormat format)
    {
        switch (format)
        {
            case CredentialFormat::KeyValuePairs:
                return "KeyValuePairs";
            case CredentialFormat::String:
                return "String";
            case CredentialFormat::FilePath:
                return "FilePath";
            case CredentialFormat::None:
            default:
                return "None";
        }
    }

    inline CredentialFormat FormatFromString(const StringPtr& format)
    {
        if (format == "KeyValuePairs")
            return CredentialFormat::KeyValuePairs;
        if (format == "String")
            return CredentialFormat::String;
        if (format == "FilePath")
            return CredentialFormat::FilePath;
        if (format == "None")
            return CredentialFormat::None;

        DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown authentication method format \"{}\"", format);
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

StructPtr AuthenticationMethodImpl::BuildKeyValueParameters(const DictPtr<IString, IBoolean>& keys)
{
    if (!keys.assigned() || keys.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value authentication method");

    return createWithImplementation<IStruct, AuthenticationMethodParametersImpl>(KeyValueAuthenticationMethodParametersStructType(),
                                                                                  Dict<IString, IBaseObject>({{"Keys", keys}}));
}

StructPtr AuthenticationMethodImpl::BuildStringParameters(Bool hidden)
{
    return createWithImplementation<IStruct, AuthenticationMethodParametersImpl>(StringAuthenticationMethodParametersStructType(),
                                                                                  Dict<IString, IBaseObject>({{"Hidden", hidden}}));
}

AuthenticationMethodImpl::AuthenticationMethodImpl(IString* id, IDict* keys, IString* description)
    : AuthenticationMethodImpl(CredentialFormat::KeyValuePairs,
                               StringPtr(id),
                               StringPtr(description),
                               BuildKeyValueParameters(DictPtr<IString, IBoolean>(keys)),
                               nullptr)
{
}

AuthenticationMethodImpl::AuthenticationMethodImpl(IString* id, IString* description, Bool hidden, IString* valuePropertyName)
    : AuthenticationMethodImpl(
          CredentialFormat::String, StringPtr(id), StringPtr(description), BuildStringParameters(hidden), StringPtr(valuePropertyName))
{
}

AuthenticationMethodImpl::AuthenticationMethodImpl(IString* id, IString* description, IString* valuePropertyName)
    : AuthenticationMethodImpl(CredentialFormat::FilePath, StringPtr(id), StringPtr(description), nullptr, StringPtr(valuePropertyName))
{
}

AuthenticationMethodImpl::AuthenticationMethodImpl(IString* id, IString* description)
    : AuthenticationMethodImpl(CredentialFormat::None, StringPtr(id), StringPtr(description), nullptr, nullptr)
{
}

AuthenticationMethodImpl::AuthenticationMethodImpl(
    CredentialFormat format, const StringPtr& id, const StringPtr& description, const StructPtr& parameters, const StringPtr& valuePropertyName)
    : format(format)
    , id(id)
    , description(description)
    , parameters(parameters)
    , valuePropertyName(valuePropertyName)
{
}

ErrCode AuthenticationMethodImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
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

    *parameters = this->parameters.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::createEmptyCredential(IPropertyObject** credential)
{
    OPENDAQ_PARAM_NOT_NULL(credential);

    if (format == CredentialFormat::None)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The \"None\" format requires no credentials, so it has no credential to build");

    auto result = PropertyObject();

    if (format == CredentialFormat::KeyValuePairs)
    {
        const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
        for (const auto& [key, hidden] : keys)
            result.addProperty(StringPropertyBuilder(key, "").build());
    }
    else
    {
        result.addProperty(StringPropertyBuilder(valuePropertyName, "").build());
    }

    *credential = result.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::serialize(ISerializer* serializer)
{
    return daqTry([&]
    {
        serializer->startTaggedObject(this);

        serializer->key("format");
        const StringPtr formatStr = detail::FormatToString(format);
        serializer->writeString(formatStr.getCharPtr(), formatStr.getLength());

        serializer->key("id");
        serializer->writeString(id.getCharPtr(), id.getLength());

        serializer->key("description");
        serializer->writeString(description.getCharPtr(), description.getLength());

        if (format == CredentialFormat::KeyValuePairs)
        {
            serializer->key("keys");
            const DictPtr<IString, IBoolean> keys = parameters.get("Keys");
            keys.serialize(SerializerPtr::Borrow(serializer));
        }
        else if (format == CredentialFormat::String)
        {
            serializer->key("hidden");
            const Bool hidden = parameters.get("Hidden");
            serializer->writeBool(hidden);
        }

        if (valuePropertyName.assigned())
        {
            serializer->key("valuePropertyName");
            serializer->writeString(valuePropertyName.getCharPtr(), valuePropertyName.getLength());
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
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
        const CredentialFormat format = detail::FormatFromString(serializedObj.readString("format"));
        const StringPtr id = serializedObj.readString("id");
        const StringPtr description = serializedObj.readString("description");

        StringPtr valuePropertyName;
        if (serializedObj.hasKey("valuePropertyName"))
            valuePropertyName = serializedObj.readString("valuePropertyName");

        AuthenticationMethodPtr result;
        switch (format)
        {
            case CredentialFormat::KeyValuePairs:
            {
                const DictPtr<IString, IBoolean> keys = serializedObj.readObject("keys", BaseObjectPtr::Borrow(context), FunctionPtr::Borrow(factoryCallback)).asPtr<IDict>();
                result = KeyValueAuthenticationMethod(id, keys, description);
                break;
            }
            case CredentialFormat::String:
            {
                const Bool hidden = serializedObj.readBool("hidden");
                result = StringAuthenticationMethod(id, description, hidden, valuePropertyName);
                break;
            }
            case CredentialFormat::FilePath:
                result = FilePathAuthenticationMethod(id, description, valuePropertyName);
                break;
            case CredentialFormat::None:
                result = NoneAuthenticationMethod(id, description);
                break;
        }

        *obj = result.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValueAuthenticationMethod, IAuthenticationMethod, IString*, id, IDict*, keys, IString*, description)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringAuthenticationMethod, IAuthenticationMethod, IString*, id, IString*, description, Bool, hidden, IString*, valuePropertyName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FilePathAuthenticationMethod, IAuthenticationMethod, IString*, id, IString*, description, IString*, valuePropertyName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NoneAuthenticationMethod, IAuthenticationMethod, IString*, id, IString*, description)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationMethodImpl)

END_NAMESPACE_OPENDAQ
