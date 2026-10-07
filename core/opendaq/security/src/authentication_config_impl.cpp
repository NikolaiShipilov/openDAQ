#include <opendaq/authentication_config_impl.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <opendaq/component_update_context_ptr.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/ctutils.h>
#include <opendaq/authentication_config_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(const DictPtr<IString, IAuthenticationMethod>& authenticationMethods)
    : Super()
{
    initProperties(authenticationMethods);
}

AuthenticationConfigImpl::AuthenticationConfigImpl()
    : Super()
{
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, IAuthenticationMethod>& authenticationMethods)
{
    if (!authenticationMethods.assigned() || authenticationMethods.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "At least one authentication method must be supplied when creating an authentication config");

    ListPtr<IStruct> authenticationMethodOptions = List<IStruct>();
    for (const auto& [id, authenticationMethod] : authenticationMethods)
        authenticationMethodOptions.pushBack(authenticationMethod);

    Super::addProperty(SelectionProperty(AuthenticationMethodPropertyName, authenticationMethodOptions, 0));
}

void AuthenticationConfigImpl::clearSuppliedCredentialIfIncompatible(const AuthenticationMethodPtr& selectedMethod)
{
    if (!objPtr.hasProperty(SuppliedCredentialPropertyName))
        return;

    const PropertyObjectPtr credential = objPtr.getPropertyValue(SuppliedCredentialPropertyName);
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

DictPtr<IString, IAuthenticationMethod> AuthenticationConfigImpl::ToAuthenticationMethodDict(const ListPtr<IStruct>& candidates)
{
    DictPtr<IString, IAuthenticationMethod> result = Dict<IString, IAuthenticationMethod>();
    for (const auto& candidate : candidates)
    {
        const auto authenticationMethod = candidate.asPtr<IAuthenticationMethod>();
        result.set(authenticationMethod.getId(), authenticationMethod);
    }
    return result;
}

ErrCode AuthenticationConfigImpl::getSelectedAuthenticationMethodId(IString** authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    return daqTry([&]
    {
        const StructPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        *authenticationMethodId = selected.asPtr<IAuthenticationMethod>().getId().detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::setAuthenticationMethodId(IString* authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    return daqTry([&]
    {
        const StringPtr idPtr = StringPtr::Borrow(authenticationMethodId);
        const ListPtr<IStruct> candidates = objPtr.getProperty(AuthenticationMethodPropertyName).getSelectionValues();
        const auto authenticationMethods = ToAuthenticationMethodDict(candidates);

        if (!authenticationMethods.hasKey(idPtr))
            DAQ_THROW_EXCEPTION(
                NotFoundException, "\"{}\" is not one of this config's supported authentication methods", idPtr);

        checkErrorInfo(this->setPropertySelectionValue(String(AuthenticationMethodPropertyName), authenticationMethods.get(idPtr)));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getSupportedAuthenticationMethods(IDict** authenticationMethods)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethods);

    return daqTry([&]
    {
        const ListPtr<IStruct> candidates = objPtr.getProperty(AuthenticationMethodPropertyName).getSelectionValues();
        *authenticationMethods = ToAuthenticationMethodDict(candidates).detach();
        return OPENDAQ_SUCCESS;
    });
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
        const AuthenticationMethodPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        clearSuppliedCredentialIfIncompatible(selected);
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
            const AuthenticationMethodPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
            if (!IsSuppliedCredentialShapeValid(credential, selected))
                DAQ_THROW_EXCEPTION(InvalidParameterException,
                                     "Supplied credential's shape does not match the currently selected authentication method \"{}\"",
                                     selected.assigned() ? selected.getId() : StringPtr(""));

            // "SuppliedCredential" is never declared up front - added here on first write.
            if (!objPtr.hasProperty(SuppliedCredentialPropertyName))
                return Super::addProperty(ObjectProperty(SuppliedCredentialPropertyName, credential));

            return Super::setProtectedPropertyValue(propertyName, value);
        });
    }

    return Super::setPropertyValue(propertyName, value);
}

// Serialization relies on the generic `IPropertyObject` mechanism for `"AuthenticationMethod"` only - every
// candidate authentication method and the selected one round-trip through it like any other property.
// `"SuppliedCredential"` (a credential) is excluded from it, never persisted at all.
ErrCode AuthenticationConfigImpl::serializeProperty(const PropertyPtr& property, ISerializer* serializer)
{
    if (property.getName() == SuppliedCredentialPropertyName)
        return OPENDAQ_SUCCESS;
    return Super::serializeProperty(property, serializer);
}

ErrCode AuthenticationConfigImpl::serializePropertyValue(const StringPtr& name, const ObjectPtr<IBaseObject>& value, ISerializer* serializer, bool forUpdate)
{
    if (name == SuppliedCredentialPropertyName)
        return OPENDAQ_SUCCESS;
    return Super::serializePropertyValue(name, value, serializer, forUpdate);
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


// Deserializing can't use the generic property-object reconstruction pipeline as a black box
// (this class has no default constructor that adds "AuthenticationMethod" up front the way its real constructor does),
// so it builds a bare stub first, then runs that same generic pipeline on it to add "AuthenticationMethod" back from its own
// serialized definition and restore its saved selection, if one was saved.
ErrCode AuthenticationConfigImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
        const auto contextObj = BaseObjectPtr::Borrow(context);
        const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

        ContextPtr daqContext;
        if (const auto deserializeContext = contextObj.asPtrOrNull<IComponentDeserializeContext>(); deserializeContext.assigned())
            daqContext = deserializeContext.getContext();
        else if (const auto updateContext = contextObj.asPtrOrNull<IComponentUpdateContext>(); updateContext.assigned())
            daqContext = updateContext.getRootComponent().getContext();

        if (!daqContext.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Unable to resolve a Context while deserializing an AuthenticationConfig");

        // A bare stub with no properties at all yet (the constructor above) - the generic PropertyObject
        // deserialization pipeline adds "AuthenticationMethod" back fresh from its own serialized definition
        // (candidates and all) and restores its saved selection override, if one was saved - both through the
        // exact same machinery any other Property goes through, no manual JSON parsing of its own needed here.
        // "SuppliedCredential" is never among "properties"/"propValues" in the first place (see
        // `serializeProperty`/`serializePropertyValue`), so these calls never touch it.

        // The generic pipeline's `context` param isn't the component deserialize context (`contextObj`) - it's
        // forwarded as-is into nested Struct deserialization (e.g. "AuthenticationMethod"'s own
        // `IAuthenticationMethod`-typed candidates), which resolves a `TypeManager` off of it directly - so it
        // must actually be one.
        const BaseObjectPtr typeManagerObj = daqContext.getTypeManager();
        PropertyObjectPtr authConfig = createWithImplementation<IAuthenticationConfig, AuthenticationConfigImpl>();
        Super::DeserializePropertyOrder(serializedObj, typeManagerObj, factoryCallbackPtr, authConfig);
        Super::DeserializeLocalProperties(serializedObj, typeManagerObj, factoryCallbackPtr, authConfig);
        Super::DeserializePropertyValues(serializedObj, typeManagerObj, factoryCallbackPtr, authConfig);

        if (!authConfig.hasProperty(AuthenticationMethodPropertyName))
            DAQ_THROW_EXCEPTION(InvalidValueException,
                                 "Serialized AuthenticationConfig is missing its \"{}\" property", AuthenticationMethodPropertyName);

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
