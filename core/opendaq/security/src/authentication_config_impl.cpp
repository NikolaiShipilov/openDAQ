#include <opendaq/authentication_config_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(IDict* payloadDescriptors,
                                                   IString* defaultPayloadId,
                                                   IList* availableCredentialProviderIds,
                                                   const StringPtr& credentialProviderId,
                                                   const PropertyObjectPtr& suppliedSecret)
    : Super()
{
    const DictPtr<IString, ICredentialPayloadDescriptor> payloadDescriptorsPtr = payloadDescriptors;
    const ListPtr<IString> availableCredentialProviderIdsPtr = availableCredentialProviderIds;
    initProperties(payloadDescriptorsPtr, defaultPayloadId, availableCredentialProviderIdsPtr, credentialProviderId, suppliedSecret);
}

AuthenticationConfigImpl::AuthenticationConfigImpl()
    : Super()
{
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, ICredentialPayloadDescriptor>& payloadDescriptors,
                                              const StringPtr& defaultPayloadId,
                                              const ListPtr<IString>& availableCredentialProviderIds,
                                              const StringPtr& credentialProviderId,
                                              const PropertyObjectPtr& suppliedSecret)
{
    if (!payloadDescriptors.assigned() || payloadDescriptors.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "At least one payload descriptor must be supplied when creating an authentication config");

    ListPtr<IStruct> payloadDescriptorOptions = List<IStruct>();
    Int defaultIndex = 0;
    Int i = 0;
    for (const auto& [id, descriptor] : payloadDescriptors)
    {
        payloadDescriptorOptions.pushBack(descriptor);
        if (defaultPayloadId.assigned() && id == defaultPayloadId)
            defaultIndex = i;
        i++;
    }

    Super::addProperty(SelectionProperty(PayloadDescriptorPropertyName, payloadDescriptorOptions, defaultIndex));

    // A real provider list (see `IDevice::createDefaultAuthenticationConfig`) makes "CredentialProviderId" a
    // selection over it. Without one - e.g. a config built via `AuthenticationConfigBuilder`, which has no
    // `Context` access to enumerate providers - it falls back to a plain string, same as before this config
    // could ever carry a provider list at all; that fallback is itself skipped (the property omitted
    // entirely) only when there is truly nothing to say - no list and no explicit id either.
    if (availableCredentialProviderIds.assigned() && availableCredentialProviderIds.getCount() > 0)
    {
        Int providerDefaultIndex = 0;
        if (credentialProviderId.assigned())
        {
            Int providerIndex = 0;
            for (const auto& id : availableCredentialProviderIds)
            {
                if (id == credentialProviderId)
                {
                    providerDefaultIndex = providerIndex;
                    break;
                }
                providerIndex++;
            }
        }

        Super::addProperty(SelectionProperty(CredentialProviderIdPropertyName, availableCredentialProviderIds, providerDefaultIndex));
    }
    else if (credentialProviderId.assigned())
    {
        Super::addProperty(StringProperty(CredentialProviderIdPropertyName, credentialProviderId));
    }

    // Only added when a secret was actually supplied - its mere presence (checked via `hasProperty`) is
    // what "was a secret supplied" means, since an Object-type property cannot itself hold `nullptr`.
    if (suppliedSecret.assigned())
        Super::addProperty(ObjectProperty(SuppliedSecretPropertyName, suppliedSecret));
}

ErrCode AuthenticationConfigImpl::getCredentialPayloadId(IString** payloadId)
{
    OPENDAQ_PARAM_NOT_NULL(payloadId);

    return daqTry([&]
    {
        const StructPtr selected = objPtr.getPropertySelectionValue(PayloadDescriptorPropertyName);
        *payloadId = selected.asPtr<ICredentialPayloadDescriptor>().getId().detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    return daqTry([&]
    {
        const StructPtr selected = objPtr.getPropertySelectionValue(PayloadDescriptorPropertyName);
        *descriptor = selected.asPtr<ICredentialPayloadDescriptor>().detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getCredentialProviderId(IString** providerId)
{
    OPENDAQ_PARAM_NOT_NULL(providerId);

    return daqTry([&]
    {
        // The property is entirely absent when this config wasn't built with either a provider list or an
        // explicit id (see `initProperties`) - that absence is itself "no provider explicitly selected".
        if (!objPtr.hasProperty(CredentialProviderIdPropertyName))
        {
            *providerId = nullptr;
            return OPENDAQ_SUCCESS;
        }

        // Shape (Selection vs plain String) is read back from the property itself, not tracked separately,
        // so this works the same whether the object was just built or restored via deserialization.
        const CoreType valueType = objPtr.getProperty(CredentialProviderIdPropertyName).getValueType();
        const StringPtr id = valueType == ctInt
                                  ? objPtr.getPropertySelectionValue(CredentialProviderIdPropertyName)
                                  : objPtr.getPropertyValue(CredentialProviderIdPropertyName);
        *providerId = (id.assigned() && id.getLength() > 0) ? id.addRefAndReturn() : nullptr;
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

    return daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = Super::DeserializePropertyObject(
                serialized,
                context,
                factoryCallback,
                [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                {
                    return createWithImplementation<IAuthenticationConfig, AuthenticationConfigImpl>();
                }).detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, payloadDescriptors, IString*, defaultPayloadId, IList*, availableCredentialProviderIds
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationConfigImpl)

END_NAMESPACE_OPENDAQ
