#include <opendaq/authentication_config_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(IDict* payloadDescriptors,
                                                   IString* defaultPayloadId,
                                                   const StringPtr& credentialProviderId,
                                                   const PropertyObjectPtr& suppliedSecret)
    : Super()
{
    const DictPtr<IString, ICredentialPayloadDescriptor> payloadDescriptorsPtr = payloadDescriptors;
    initProperties(payloadDescriptorsPtr, defaultPayloadId, credentialProviderId, suppliedSecret);
}

AuthenticationConfigImpl::AuthenticationConfigImpl()
    : Super()
{
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, ICredentialPayloadDescriptor>& payloadDescriptors,
                                              const StringPtr& defaultPayloadId,
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
    Super::addProperty(StringProperty(CredentialProviderIdPropertyName, credentialProviderId.assigned() ? credentialProviderId : ""));

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
        const StringPtr id = objPtr.getPropertyValue(CredentialProviderIdPropertyName);
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
    IDict*, payloadDescriptors, IString*, defaultPayloadId
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationConfigImpl)

END_NAMESPACE_OPENDAQ
