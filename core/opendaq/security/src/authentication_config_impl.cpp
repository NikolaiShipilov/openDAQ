#include <opendaq/authentication_config_impl.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/serializer_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/ctutils.h>
#include <opendaq/authentication_config_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(const DictPtr<IString, IAuthenticationMethod>& authenticationMethods)
    : Super()
{
    initProperties(authenticationMethods);
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, IAuthenticationMethod>& authenticationMethods)
{
    if (!authenticationMethods.assigned() || authenticationMethods.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "At least one authentication method must be supplied when creating an authentication config");

    ListPtr<IString> authenticationMethodIds = List<IString>();
    for (const auto& [id, authenticationMethod] : authenticationMethods)
        authenticationMethodIds.pushBack(id);

    Super::addProperty(SelectionProperty(AuthenticationMethodPropertyName, authenticationMethodIds, 0));
    this->supportedAuthenticationMethods = authenticationMethods;
}

void AuthenticationConfigImpl::clearSuppliedCredentialIfIncompatible(const StringPtr& selectedMethodId)
{
    if (!objPtr.hasProperty(SuppliedCredentialPropertyName))
        return;

    const PropertyObjectPtr credential = objPtr.getPropertyValue(SuppliedCredentialPropertyName);
    const AuthenticationMethodPtr selectedMethod =
        selectedMethodId.assigned() ? supportedAuthenticationMethods.getOrDefault(selectedMethodId) : nullptr;
    if (!IsSuppliedCredentialShapeValid(credential, selectedMethod))
        Super::removeProperty(String(SuppliedCredentialPropertyName));
}

bool AuthenticationConfigImpl::IsSuppliedCredentialShapeValid(const PropertyObjectPtr& credential, const AuthenticationMethodPtr& selectedMethod)
{
    if (!credential.assigned() || !selectedMethod.assigned())
        return false;

    // "None" requires no credentials at all - no credential is ever valid for it, and it has no
    // `createEmptyCredential()` template to compare against in the first place.
    if (selectedMethod.getFormat() == CredentialFormat::None)
        return false;

    const PropertyObjectPtr templateObj = selectedMethod.createEmptyCredential();
    const auto templateProps = templateObj.getAllProperties();

    if (templateProps.getCount() != credential.getAllProperties().getCount())
        return false;

    for (const auto& prop : templateProps)
    {
        if (!credential.hasProperty(prop.getName()))
            return false;
    }

    return true;
}

ErrCode AuthenticationConfigImpl::getSelectedAuthenticationMethodId(IString** authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    return daqTry([&]
    {
        StringPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        *authenticationMethodId = selected.detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::setAuthenticationMethodId(IString* authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    return daqTry([&]
    {
        const StringPtr idPtr = StringPtr::Borrow(authenticationMethodId);

        if (!supportedAuthenticationMethods.hasKey(idPtr))
            DAQ_THROW_EXCEPTION(
                NotFoundException, "\"{}\" is not one of this config's supported authentication methods", idPtr);

        checkErrorInfo(this->setPropertySelectionValue(String(AuthenticationMethodPropertyName), idPtr));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getSupportedAuthenticationMethods(IDict** authenticationMethods)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethods);

    *authenticationMethods = this->supportedAuthenticationMethods.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::getSuppliedCredential(IPropertyObject** credential)
{
    OPENDAQ_PARAM_NOT_NULL(credential);

    return daqTry([&]
    {
        // Present only when the caller actually set one - an Object-type property cannot itself hold
        // `nullptr`, so absence of the property is the only way to represent "none supplied".
        *credential = objPtr.hasProperty(SuppliedCredentialPropertyName)
                      ? PropertyObjectPtr(objPtr.getPropertyValue(SuppliedCredentialPropertyName)).detach()
                      : nullptr;
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    OPENDAQ_PARAM_NOT_NULL(idCount);

    *idCount = InterfaceIds::Count() + 1;
    if (ids == nullptr)
        return OPENDAQ_SUCCESS;

    **ids = IPropertyObject::Id;
    (*ids)++;

    InterfaceIds::AddInterfaceIds(*ids);
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::setPropertySelectionValue(IString* propertyName, IBaseObject* value)
{
    const ErrCode errCode = Super::setPropertySelectionValue(propertyName, value);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return onPropertyValueChanged(StringPtr::Borrow(propertyName));
}

ErrCode AuthenticationConfigImpl::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    const ErrCode errCode = Super::setProtectedPropertyValue(propertyName, value);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return onPropertyValueChanged(StringPtr::Borrow(propertyName));
}

ErrCode AuthenticationConfigImpl::onPropertyValueChanged(const StringPtr& name)
{
    if (name != AuthenticationMethodPropertyName)
        return OPENDAQ_SUCCESS;

    return daqTry([&]
    {
        const StringPtr selectedId = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        clearSuppliedCredentialIfIncompatible(selectedId);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    const StringPtr name = StringPtr::Borrow(propertyName);

    if (name == SuppliedCredentialPropertyName)
    {
        return daqTry([&]
        {
            const PropertyObjectPtr credential = BaseObjectPtr::Borrow(value).asPtrOrNull<IPropertyObject>();
            const StringPtr selectedId = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
            const AuthenticationMethodPtr selected = selectedId.assigned() ? supportedAuthenticationMethods.getOrDefault(selectedId) : nullptr;
            if (!IsSuppliedCredentialShapeValid(credential, selected))
                DAQ_THROW_EXCEPTION(InvalidParameterException,
                                     "Supplied credential's shape does not match the currently selected authentication method \"{}\"",
                                     selectedId.assigned() ? selectedId : StringPtr(""));

            // "SuppliedCredential" is never declared up front - added here on first write.
            if (!objPtr.hasProperty(SuppliedCredentialPropertyName))
                return Super::addProperty(ObjectProperty(SuppliedCredentialPropertyName, credential));

            return Super::setProtectedPropertyValue(propertyName, value);
        });
    }

    return Super::setPropertyValue(propertyName, value);
}

// Serialization is entirely custom - replaces the inherited generic `IPropertyObject` one, mirroring
// `DeviceTypeImpl`'s own pattern. Writes only `supportedAuthenticationMethods` (the real `IAuthenticationMethod`
// objects this config was built from) and the currently selected id - `"SuppliedCredential"` is never written.
ErrCode AuthenticationConfigImpl::serialize(ISerializer* serializer)
{
    return daqTry([&]
    {
        serializer->startTaggedObject(this);

        serializer->key(SupportedAuthenticationMethodsSerializedKey);
        supportedAuthenticationMethods.serialize(SerializerPtr::Borrow(serializer));

        const StringPtr selectedId = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        serializer->key("selectedAuthenticationMethodId");
        serializer->writeString(selectedId.getCharPtr(), selectedId.getLength());

        serializer->endObject();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr AuthenticationConfigImpl::SerializeId()
{
    return "AuthenticationConfig";
}

// Rebuilds the config directly from its two serialized pieces, via the real constructor, then restores the
// saved selection - fully self-contained, no module/type registry re-consultation needed. Throws if the saved
// selected id is no longer among the (also saved) supported methods - consistent with "no compatible method"
// already being a hard failure elsewhere in this framework.
ErrCode AuthenticationConfigImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);

        const DictPtr<IString, IAuthenticationMethod> supportedAuthenticationMethods =
            serializedObj.readObject(SupportedAuthenticationMethodsSerializedKey, BaseObjectPtr::Borrow(context), FunctionPtr::Borrow(factoryCallback))
                .asPtr<IDict>();
        const StringPtr selectedId = serializedObj.readString("selectedAuthenticationMethodId");

        AuthenticationConfigPtr authConfig = createWithImplementation<IAuthenticationConfig, AuthenticationConfigImpl>(supportedAuthenticationMethods);
        authConfig.setAuthenticationMethodId(selectedId);

        *obj = authConfig.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, authenticationMethods
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationConfigImpl)

END_NAMESPACE_OPENDAQ
