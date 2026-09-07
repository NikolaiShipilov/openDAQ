#include <opendaq/authentication_config_impl.h>
#include <opendaq/authentication_config_factory.h>
#include <opendaq/component_deserialize_context.h>
#include <opendaq/credential_provider_ptr.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(const DictPtr<IString, ICredentialPayloadDescriptor>& payloadDescriptors,
                                                   const StringPtr& defaultPayloadId,
                                                   const ContextPtr& context,
                                                   const StringPtr& typeId)
    : Super()
    , context(context)
    , typeId(typeId)
{
    initProperties(payloadDescriptors, defaultPayloadId);
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, ICredentialPayloadDescriptor>& payloadDescriptors,
                                              const StringPtr& defaultPayloadId)
{
    if (!payloadDescriptors.assigned() || payloadDescriptors.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "At least one payload descriptor must be supplied when creating an authentication config");

    ListPtr<IStruct> payloadDescriptorOptions = List<IStruct>();
    Int defaultIndex = 0;
    Int i = 0;
    CredentialPayloadDescriptorPtr selectedDescriptor;
    for (const auto& [id, descriptor] : payloadDescriptors)
    {
        payloadDescriptorOptions.pushBack(descriptor);
        if (defaultPayloadId.assigned() && id == defaultPayloadId)
        {
            defaultIndex = i;
            selectedDescriptor = descriptor;
        }
        i++;
    }
    if (!selectedDescriptor.assigned())
    {
        for (const auto& [id, descriptor] : payloadDescriptors)
        {
            selectedDescriptor = descriptor;
            break;
        }
    }

    Super::addProperty(SelectionProperty(PayloadDescriptorPropertyName, payloadDescriptorOptions, defaultIndex));
    rebuildCredentialProviderCandidates(selectedDescriptor);
}

void AuthenticationConfigImpl::rebuildCredentialProviderCandidates(const CredentialPayloadDescriptorPtr& selectedDescriptor)
{
    // No live provider list to filter without a `Context` - "CredentialProviderId" is simply never present.
    if (!context.assigned())
        return;

    if (objPtr.hasProperty(CredentialProviderIdPropertyName))
        Super::removeProperty(CredentialProviderIdPropertyName);

    if (!selectedDescriptor.assigned())
        return;

    ListPtr<IString> compatibleProviderIds = List<IString>();
    for (const auto& [providerId, provider] : context.getCredentialProviders())
    {
        for (const auto& format : provider.getSupportedPayloadFormats())
        {
            if (static_cast<CredentialPayloadFormat>(static_cast<Int>(format)) == selectedDescriptor.getFormat())
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

void AuthenticationConfigImpl::clearSuppliedSecretIfIncompatible(const CredentialPayloadDescriptorPtr& selectedDescriptor)
{
    if (!objPtr.hasProperty(SuppliedSecretPropertyName))
        return;

    const PropertyObjectPtr secret = objPtr.getPropertyValue(SuppliedSecretPropertyName);
    if (!IsSuppliedSecretShapeValid(secret, selectedDescriptor))
        Super::removeProperty(SuppliedSecretPropertyName);
}

bool AuthenticationConfigImpl::IsSuppliedSecretShapeValid(const PropertyObjectPtr& secret, const CredentialPayloadDescriptorPtr& selectedDescriptor)
{
    if (!secret.assigned() || !selectedDescriptor.assigned())
        return false;

    const PropertyObjectPtr templateObj = selectedDescriptor.createDefaultPayload();
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
        // The property is entirely absent when this config has no `Context` or no compatible provider for
        // the currently selected format (see `rebuildCredentialProviderCandidates`) - that absence is itself
        // "no provider explicitly selected".
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

ErrCode AuthenticationConfigImpl::setPropertySelectionValue(IString* propertyName, IBaseObject* value)
{
    const ErrCode errCode = Super::setPropertySelectionValue(propertyName, value);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    const StringPtr name = StringPtr::Borrow(propertyName);

    if (name == PayloadDescriptorPropertyName)
    {
        return daqTry([&]
        {
            const CredentialPayloadDescriptorPtr selected = objPtr.getPropertySelectionValue(PayloadDescriptorPropertyName);
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
            const CredentialPayloadDescriptorPtr selected = objPtr.getPropertySelectionValue(PayloadDescriptorPropertyName);
            if (!IsSuppliedSecretShapeValid(secret, selected))
                DAQ_THROW_EXCEPTION(InvalidParameterException,
                                     "Supplied secret's shape does not match the currently selected payload descriptor \"{}\"",
                                     selected.assigned() ? selected.getId() : StringPtr(""));

            return Super::setPropertyValue(propertyName, value);
        });
    }

    return Super::setPropertyValue(propertyName, value);
}

ErrCode AuthenticationConfigImpl::serialize(ISerializer* serializer)
{
    return daqTry([&]
    {
        const StructPtr selected = objPtr.getPropertySelectionValue(PayloadDescriptorPropertyName);
        const StringPtr payloadId = selected.asPtr<ICredentialPayloadDescriptor>().getId();

        serializer->startTaggedObject(this);
        serializer->key(TypeIdSerializedKey);
        serializer->writeString(typeId.getCharPtr(), typeId.getLength());
        serializer->key(PayloadIdSerializedKey);
        serializer->writeString(payloadId.getCharPtr(), payloadId.getLength());
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
        const StringPtr savedPayloadId = serializedObj.readString(PayloadIdSerializedKey);

        const auto deserializeContext = BaseObjectPtr::Borrow(context).asPtr<IComponentDeserializeContext>(true);
        const ContextPtr realContext = deserializeContext.getContext();

        const ModuleManagerUtilsPtr managerUtils = realContext.getModuleManager().asPtr<IModuleManagerUtils>();
        ComponentTypePtr componentType;
        if (const auto deviceTypes = managerUtils.getAvailableDeviceTypes(); deviceTypes.hasKey(savedTypeId))
            componentType = deviceTypes.get(savedTypeId);
        else if (const auto streamingTypes = managerUtils.getAvailableStreamingTypes(); streamingTypes.hasKey(savedTypeId))
            componentType = streamingTypes.get(savedTypeId);
        else
            DAQ_THROW_EXCEPTION(NotFoundException, "No available device or streaming type with id \"{}\" was found", savedTypeId);

        const DictPtr<IString, ICredentialPayloadDescriptor> descriptors = componentType.getSupportedAuthenticationDescriptors();
        if (!descriptors.hasKey(savedPayloadId))
            DAQ_THROW_EXCEPTION(NotSupportedException,
                                 "Saved payload id \"{}\" is no longer supported by type \"{}\"",
                                 savedPayloadId,
                                 savedTypeId);

        *obj = AuthenticationConfig(descriptors, savedPayloadId, realContext, savedTypeId).detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, payloadDescriptors, IString*, defaultPayloadId, IContext*, context, IString*, typeId
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationConfigImpl)

END_NAMESPACE_OPENDAQ
