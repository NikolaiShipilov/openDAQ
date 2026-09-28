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

AuthenticationConfigImpl::AuthenticationConfigImpl(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors)
    : Super()
{
    initProperties(credentialDescriptors);
}

AuthenticationConfigImpl::AuthenticationConfigImpl()
    : Super()
{
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors)
{
    if (!credentialDescriptors.assigned() || credentialDescriptors.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "At least one credential descriptor must be supplied when creating an authentication config");

    ListPtr<IStruct> credentialDescriptorOptions = List<IStruct>();
    for (const auto& [id, descriptor] : credentialDescriptors)
        credentialDescriptorOptions.pushBack(descriptor);

    Super::addProperty(SelectionProperty(AuthenticationMethodPropertyName, credentialDescriptorOptions, 0));
}

void AuthenticationConfigImpl::clearSuppliedSecretIfIncompatible(const CredentialDescriptorPtr& selectedDescriptor)
{
    if (!objPtr.hasProperty(SuppliedSecretPropertyName))
        return;

    const PropertyObjectPtr secret = objPtr.getPropertyValue(SuppliedSecretPropertyName);
    if (!IsSuppliedSecretShapeValid(secret, selectedDescriptor))
        Super::removeProperty(String(SuppliedSecretPropertyName));
}

bool AuthenticationConfigImpl::IsSuppliedSecretShapeValid(const PropertyObjectPtr& secret, const CredentialDescriptorPtr& selectedDescriptor)
{
    if (!secret.assigned() || !selectedDescriptor.assigned())
        return false;

    // "None" requires no credentials at all - no secret is ever valid for it, and it has no
    // `createEmptySecret()` template to compare against in the first place.
    if (selectedDescriptor.getFormat() == CredentialFormat::None)
        return false;

    const PropertyObjectPtr templateObj = selectedDescriptor.createEmptySecret();
    const auto templateProps = templateObj.getAllProperties();

    if (templateProps.getCount() != secret.getAllProperties().getCount())
        return false;

    for (const auto& prop : templateProps)
    {
        if (!secret.hasProperty(prop.getName()))
            return false;
    }

    return true;
}

DictPtr<IString, ICredentialDescriptor> AuthenticationConfigImpl::ToCredentialDescriptorDict(const ListPtr<IStruct>& candidates)
{
    DictPtr<IString, ICredentialDescriptor> result = Dict<IString, ICredentialDescriptor>();
    for (const auto& candidate : candidates)
    {
        const auto descriptor = candidate.asPtr<ICredentialDescriptor>();
        result.set(descriptor.getAuthenticationMethodId(), descriptor);
    }
    return result;
}

ErrCode AuthenticationConfigImpl::getSelectedAuthenticationMethodId(IString** authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    return daqTry([&]
    {
        const StructPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        *authenticationMethodId = selected.asPtr<ICredentialDescriptor>().getAuthenticationMethodId().detach();
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
        const auto descriptors = ToCredentialDescriptorDict(candidates);

        if (!descriptors.hasKey(idPtr))
            DAQ_THROW_EXCEPTION(
                NotFoundException, "\"{}\" is not one of this config's supported authentication methods", idPtr);

        checkErrorInfo(this->setPropertySelectionValue(String(AuthenticationMethodPropertyName), descriptors.get(idPtr)));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getSupportedAuthenticationMethods(IDict** descriptors)
{
    OPENDAQ_PARAM_NOT_NULL(descriptors);

    return daqTry([&]
    {
        const ListPtr<IStruct> candidates = objPtr.getProperty(AuthenticationMethodPropertyName).getSelectionValues();
        *descriptors = ToCredentialDescriptorDict(candidates).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getSuppliedSecret(IPropertyObject** secret)
{
    OPENDAQ_PARAM_NOT_NULL(secret);

    return daqTry([&]
    {
        // Present only when the caller actually set one - an Object-type property cannot itself hold
        // `nullptr`, so absence of the property is the only way to represent "none supplied".
        *secret = objPtr.hasProperty(SuppliedSecretPropertyName)
                      ? PropertyObjectPtr(objPtr.getPropertyValue(SuppliedSecretPropertyName)).detach()
                      : nullptr;
        return OPENDAQ_SUCCESS;
    });
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
        const CredentialDescriptorPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        clearSuppliedSecretIfIncompatible(selected);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    const StringPtr name = StringPtr::Borrow(propertyName);

    if (name == SuppliedSecretPropertyName)
    {
        return daqTry([&]
        {
            const PropertyObjectPtr secret = BaseObjectPtr::Borrow(value).asPtrOrNull<IPropertyObject>();
            const CredentialDescriptorPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
            if (!IsSuppliedSecretShapeValid(secret, selected))
                DAQ_THROW_EXCEPTION(InvalidParameterException,
                                     "Supplied secret's shape does not match the currently selected credential descriptor \"{}\"",
                                     selected.assigned() ? selected.getAuthenticationMethodId() : StringPtr(""));

            // "SuppliedSecret" is never declared up front - added here on first write.
            if (!objPtr.hasProperty(SuppliedSecretPropertyName))
                return Super::addProperty(ObjectProperty(SuppliedSecretPropertyName, secret));

            return Super::setPropertyValue(propertyName, value);
        });
    }

    return Super::setPropertyValue(propertyName, value);
}

// Serialization relies on the generic `IPropertyObject` mechanism for `"AuthenticationMethod"` only - every
// candidate credential descriptor and the selected one round-trip through it like any other property.
// `"SuppliedSecret"` (a secret) is excluded from it, never persisted at all.
ErrCode AuthenticationConfigImpl::serializeProperty(const PropertyPtr& property, ISerializer* serializer)
{
    if (property.getName() == SuppliedSecretPropertyName)
        return OPENDAQ_SUCCESS;
    return Super::serializeProperty(property, serializer);
}

ErrCode AuthenticationConfigImpl::serializePropertyValue(const StringPtr& name, const ObjectPtr<IBaseObject>& value, ISerializer* serializer, bool forUpdate)
{
    if (name == SuppliedSecretPropertyName)
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
        // "SuppliedSecret" is never among "properties"/"propValues" in the first place (see
        // `serializeProperty`/`serializePropertyValue`), so these calls never touch it.

        // The generic pipeline's `context` param isn't the component deserialize context (`contextObj`) - it's
        // forwarded as-is into nested Struct deserialization (e.g. "AuthenticationMethod"'s own
        // `ICredentialDescriptor`-typed candidates), which resolves a `TypeManager` off of it directly - so it
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
    IDict*, credentialDescriptors
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationConfigImpl)

END_NAMESPACE_OPENDAQ
