#include <opendaq/authentication_config_impl.h>
#include <opendaq/authentication_config_factory.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <opendaq/component_update_context_ptr.h>
#include <opendaq/credential_provider_ptr.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/ctutils.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors,
                                                   const StringPtr& defaultAuthenticationMethodId,
                                                   const ContextPtr& context,
                                                   const StringPtr& typeId)
    : Super()
    , context(context)
    , typeId(typeId)
{
    initProperties(credentialDescriptors, defaultAuthenticationMethodId);
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors,
                                              const StringPtr& defaultAuthenticationMethodId)
{
    if (!context.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Context must be assigned when creating an authentication config");

    if (!credentialDescriptors.assigned() || credentialDescriptors.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "At least one credential descriptor must be supplied when creating an authentication config");

    ListPtr<IStruct> credentialDescriptorOptions = List<IStruct>();
    Int defaultIndex = 0;
    Int i = 0;
    CredentialDescriptorPtr selectedDescriptor;
    for (const auto& [id, descriptor] : credentialDescriptors)
    {
        credentialDescriptorOptions.pushBack(descriptor);
        if (defaultAuthenticationMethodId.assigned() && id == defaultAuthenticationMethodId)
        {
            defaultIndex = i;
            selectedDescriptor = descriptor;
        }
        i++;
    }
    if (!selectedDescriptor.assigned())
    {
        for (const auto& [id, descriptor] : credentialDescriptors)
        {
            selectedDescriptor = descriptor;
            break;
        }
    }

    Super::addProperty(SelectionProperty(AuthenticationMethodPropertyName, credentialDescriptorOptions, defaultIndex));
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

ErrCode AuthenticationConfigImpl::getAuthenticationMethodId(IString** authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    return daqTry([&]
    {
        const StructPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        *authenticationMethodId = selected.asPtr<ICredentialDescriptor>().getAuthenticationMethodId().detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getCredentialDescriptor(ICredentialDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    return daqTry([&]
    {
        const StructPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        *descriptor = selected.asPtr<ICredentialDescriptor>().detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getCredentialProviderId(IString** providerId)
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
        *providerId = (id.assigned() && id.getLength() > 0) ? id.addRefAndReturn() : nullptr;
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

    const StringPtr name = StringPtr::Borrow(propertyName);

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

    return errCode;
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

ErrCode AuthenticationConfigImpl::serialize(ISerializer* serializer)
{
    return daqTry([&]
    {
        const ListPtr<IStruct> credentialDescriptors = objPtr.getProperty(AuthenticationMethodPropertyName).getSelectionValues();
        const StructPtr selected = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        const StringPtr authenticationMethodId = selected.asPtr<ICredentialDescriptor>().getAuthenticationMethodId();

        serializer->startTaggedObject(this);
        serializer->key(TypeIdSerializedKey);
        serializer->writeString(typeId.getCharPtr(), typeId.getLength());
        serializer->key(CredentialDescriptorsSerializedKey);
        checkErrorInfo(credentialDescriptors->serialize(serializer));
        serializer->key(AuthenticationMethodIdSerializedKey);
        serializer->writeString(authenticationMethodId.getCharPtr(), authenticationMethodId.getLength());

        if (objPtr.hasProperty(CredentialProviderIdPropertyName))
        {
            const StringPtr providerId = objPtr.getPropertySelectionValue(CredentialProviderIdPropertyName);
            if (providerId.assigned())
            {
                serializer->key(ProviderIdSerializedKey);
                serializer->writeString(providerId.getCharPtr(), providerId.getLength());
            }
        }

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

ErrCode AuthenticationConfigImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context]
    {
        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
        const StringPtr savedTypeId = serializedObj.readString(TypeIdSerializedKey);
        const StringPtr savedAuthenticationMethodId = serializedObj.readString(AuthenticationMethodIdSerializedKey);

        const auto contextObj = BaseObjectPtr::Borrow(context);
        ContextPtr daqContext;
        if (const auto deserializeContext = contextObj.asPtrOrNull<IComponentDeserializeContext>(); deserializeContext.assigned())
            daqContext = deserializeContext.getContext();
        else if (const auto updateContext = contextObj.asPtrOrNull<IComponentUpdateContext>(); updateContext.assigned())
            daqContext = updateContext.getRootComponent().getContext();

        if (!daqContext.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Unable to resolve a Context while deserializing an AuthenticationConfig");

        const ListPtr<ICredentialDescriptor> savedDescriptors =
            serializedObj.readList<ICredentialDescriptor>(CredentialDescriptorsSerializedKey, daqContext.getTypeManager());

        DictPtr<IString, ICredentialDescriptor> credentialDescriptors = Dict<IString, ICredentialDescriptor>();
        for (const auto& descriptor : savedDescriptors)
            credentialDescriptors.set(descriptor.getAuthenticationMethodId(), descriptor);

        if (!credentialDescriptors.hasKey(savedAuthenticationMethodId))
            DAQ_THROW_EXCEPTION(NotSupportedException,
                                 "Saved authentication method id \"{}\" is not among the saved credential descriptors for type \"{}\"",
                                 savedAuthenticationMethodId,
                                 savedTypeId);

        AuthenticationConfigPtr authConfig = AuthenticationConfig(credentialDescriptors, savedAuthenticationMethodId, daqContext, savedTypeId);

        if (serializedObj.hasKey(ProviderIdSerializedKey) && authConfig.hasProperty(CredentialProviderIdPropertyName))
        {
            const StringPtr savedProviderId = serializedObj.readString(ProviderIdSerializedKey);
            const ListPtr<IString> candidates = authConfig.getProperty(CredentialProviderIdPropertyName).getSelectionValues();

            bool isCompatible = false;
            for (const auto& candidate : candidates)
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
    IDict*, credentialDescriptors, IString*, defaultAuthenticationMethodId, IContext*, context, IString*, typeId
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationConfigImpl)

END_NAMESPACE_OPENDAQ
