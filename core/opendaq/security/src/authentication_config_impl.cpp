#include <opendaq/authentication_config_impl.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <opendaq/component_update_context_ptr.h>
#include <opendaq/credential_provider_ptr.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/ctutils.h>
#include <opendaq/authentication_config_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors,
                                                   const ContextPtr& context)
    : Super()
    , context(context)
{
    initProperties(credentialDescriptors);
}

AuthenticationConfigImpl::AuthenticationConfigImpl(const ContextPtr& context)
    : Super()
    , context(context)
{
    if (!context.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Context must be assigned when creating an authentication config");
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors)
{
    if (!context.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Context must be assigned when creating an authentication config");

    if (!credentialDescriptors.assigned() || credentialDescriptors.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "At least one credential descriptor must be supplied when creating an authentication config");

    ListPtr<IStruct> credentialDescriptorOptions = List<IStruct>();
    CredentialDescriptorPtr selectedDescriptor;
    for (const auto& [id, descriptor] : credentialDescriptors)
    {
        credentialDescriptorOptions.pushBack(descriptor);
        if (!selectedDescriptor.assigned())
            selectedDescriptor = descriptor;
    }

    Super::addProperty(SelectionProperty(AuthenticationMethodPropertyName, credentialDescriptorOptions, 0));
    rebuildCredentialProviderCandidates(selectedDescriptor);
}

void AuthenticationConfigImpl::rebuildCredentialProviderCandidates(const CredentialDescriptorPtr& selectedDescriptor)
{
    if (objPtr.hasProperty(CredentialProviderIdPropertyName))
        Super::removeProperty(String(CredentialProviderIdPropertyName));

    if (!selectedDescriptor.assigned())
        return;

    ListPtr<IString> compatibleProviderIds = List<IString>();
    for (const auto& [providerId, provider] : context.getCredentialProviders())
    {
        for (const auto& format : provider.getSupportedFormats())
        {
            if (static_cast<CredentialFormat>(static_cast<Int>(format)) == selectedDescriptor.getFormat())
            {
                compatibleProviderIds.pushBack(providerId);
                break;
            }
        }
    }

    if (compatibleProviderIds.getCount() == 0)
        return;

    Int defaultIndex = 0;
    if (preferredCredentialProviderId.assigned())
    {
        Int idx = 0;
        for (const auto& id : compatibleProviderIds)
        {
            if (id == preferredCredentialProviderId)
            {
                defaultIndex = idx;
                break;
            }
            idx++;
        }
    }

    Super::addProperty(SelectionProperty(CredentialProviderIdPropertyName, compatibleProviderIds, defaultIndex));
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

ErrCode AuthenticationConfigImpl::getSelectedCredentialProviderId(IString** providerId)
{
    OPENDAQ_PARAM_NOT_NULL(providerId);

    return daqTry([&]
    {
        // The property is entirely absent when no compatible provider is registered for the currently
        // selected format (see `rebuildCredentialProviderCandidates`) - that absence is itself "no provider
        // explicitly selected".
        if (!objPtr.hasProperty(CredentialProviderIdPropertyName))
        {
            *providerId = nullptr;
            return OPENDAQ_SUCCESS;
        }

        const StringPtr id = objPtr.getPropertySelectionValue(CredentialProviderIdPropertyName);
        *providerId = id.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::setCredentialProviderId(IString* providerId)
{
    OPENDAQ_PARAM_NOT_NULL(providerId);

    return daqTry([&]
    {
        if (!objPtr.hasProperty(CredentialProviderIdPropertyName))
            DAQ_THROW_EXCEPTION(
                NotFoundException,
                "This config currently has no credential provider candidates for the selected authentication method");

        checkErrorInfo(this->setPropertySelectionValue(String(CredentialProviderIdPropertyName), providerId));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getSupportedCredentialProviderIds(IList** providerIds)
{
    OPENDAQ_PARAM_NOT_NULL(providerIds);

    return daqTry([&]
    {
        if (!objPtr.hasProperty(CredentialProviderIdPropertyName))
        {
            *providerIds = List<IString>().detach();
            return OPENDAQ_SUCCESS;
        }

        ListPtr<IString> candidates = objPtr.getProperty(CredentialProviderIdPropertyName).getSelectionValues();
        *providerIds = candidates.detach();
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
    if (name == AuthenticationMethodPropertyName)
    {
        return daqTry([&]
        {
            const CredentialDescriptorPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
            rebuildCredentialProviderCandidates(selected);
            clearSuppliedSecretIfIncompatible(selected);
            return OPENDAQ_SUCCESS;
        });
    }

    if (name == CredentialProviderIdPropertyName && context.assigned())
    {
        return daqTry([&]
        {
            const StringPtr selectedProviderId = objPtr.getPropertySelectionValue(CredentialProviderIdPropertyName);
            preferredCredentialProviderId = selectedProviderId;
            return OPENDAQ_SUCCESS;
        });
    }

    return OPENDAQ_SUCCESS;
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

ErrCode AuthenticationConfigImpl::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    const ErrCode errCode = Super::serializeCustomValues(serializer, forUpdate);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    // "CredentialProviderId"'s *selected* value is written here, as an extra value, rather than through the
    // generic mechanism, this one's candidates never persisted - always live-recomputed from the current `Context`.
    return daqTry([&]
    {
        if (objPtr.hasProperty(CredentialProviderIdPropertyName))
        {
            const StringPtr providerId = objPtr.getPropertySelectionValue(CredentialProviderIdPropertyName);
            if (providerId.assigned())
            {
                serializer->key(CredentialProviderIdPropertyName);
                serializer->writeString(providerId.getCharPtr(), providerId.getLength());
            }
        }

        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::serializeProperty(const PropertyPtr& property, ISerializer* serializer)
{
    if (property.getName() == SuppliedSecretPropertyName || property.getName() == CredentialProviderIdPropertyName)
        return OPENDAQ_SUCCESS;
    return Super::serializeProperty(property, serializer);
}

ErrCode AuthenticationConfigImpl::serializePropertyValue(const StringPtr& name, const ObjectPtr<IBaseObject>& value, ISerializer* serializer, bool forUpdate)
{
    if (name == SuppliedSecretPropertyName || name == CredentialProviderIdPropertyName)
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
        // "CredentialProviderId"/"SuppliedSecret" are never among "properties"/"propValues" in the first place
        // (see `serializeProperty`/`serializePropertyValue`), so these calls never touch them.

        // The generic pipeline's `context` param isn't the component deserialize context (`contextObj`) - it's
        // forwarded as-is into nested Struct deserialization (e.g. "AuthenticationMethod"'s own
        // `ICredentialDescriptor`-typed candidates), which resolves a `TypeManager` off of it directly - so it
        // must actually be one.
        const BaseObjectPtr typeManagerObj = daqContext.getTypeManager();
        PropertyObjectPtr authConfig = createWithImplementation<IAuthenticationConfig, AuthenticationConfigImpl>(daqContext);
        Super::DeserializePropertyOrder(serializedObj, typeManagerObj, factoryCallbackPtr, authConfig);
        Super::DeserializeLocalProperties(serializedObj, typeManagerObj, factoryCallbackPtr, authConfig);
        Super::DeserializePropertyValues(serializedObj, typeManagerObj, factoryCallbackPtr, authConfig);

        if (!authConfig.hasProperty(AuthenticationMethodPropertyName))
            DAQ_THROW_EXCEPTION(InvalidValueException,
                                 "Serialized AuthenticationConfig is missing its \"{}\" property", AuthenticationMethodPropertyName);

        // "CredentialProviderId" only gets (re)built as a side effect of an "AuthenticationMethod" write (see
        // `onPropertyValueChanged`) - which only actually happened above if a selection override was saved.
        // Re-selecting whatever ended up selected (the restored override, or the freshly-added default)
        // unconditionally guarantees that side effect runs either way.
        const AuthenticationConfigPtr authConfigTyped = authConfig.asPtr<IAuthenticationConfig>();
        authConfigTyped.setAuthenticationMethodId(authConfigTyped.getSelectedAuthenticationMethodId());

        // The saved "CredentialProviderId" selection, if any (see `serializeCustomValues` for where it's
        // written) - reapplied only if still among the freshly (live) rebuilt candidates just computed above.
        // Its saved *candidates* are never read at all - by design, see the class doc comment - only which one
        // (if any) was selected.
        if (serializedObj.hasKey(CredentialProviderIdPropertyName) && authConfig.hasProperty(CredentialProviderIdPropertyName))
        {
            const StringPtr savedProviderId = serializedObj.readString(CredentialProviderIdPropertyName);
            const ListPtr<IString> providerCandidates = authConfig.getProperty(CredentialProviderIdPropertyName).getSelectionValues();

            bool isCompatible = false;
            for (const auto& candidate : providerCandidates)
            {
                if (candidate == savedProviderId)
                {
                    isCompatible = true;
                    break;
                }
            }

            if (isCompatible)
                authConfig.setPropertySelectionValue(CredentialProviderIdPropertyName, savedProviderId);
        }

        *obj = authConfig.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, credentialDescriptors, IContext*, context
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationConfigImpl)

END_NAMESPACE_OPENDAQ
