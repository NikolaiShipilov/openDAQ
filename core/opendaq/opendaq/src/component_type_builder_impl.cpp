#include <opendaq/component_type_builder_impl.h>
#include <opendaq/server_type_impl.h>
#include <opendaq/device_type_impl.h>
#include <opendaq/function_block_type_impl.h>
#include <opendaq/streaming_type_impl.h>
#include <coretypes/validation.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

ComponentTypeBuilderImpl::ComponentTypeBuilderImpl(ComponentTypeSort sort)
    : sort(sort)
    , supportedPayloads(Dict<IString, ICredentialPayloadDescriptor>())
{
}

ErrCode ComponentTypeBuilderImpl::build(IComponentType** componentType)
{
    const ErrCode errCode = daqTry([&componentType, this]
    {
        OPENDAQ_RETURN_IF_FAILED(validateAuthenticationCapabilities());

        const auto builderPtr = this->borrowPtr<ComponentTypeBuilderPtr>();
        switch (sort)
        {
            case ComponentTypeSort::Server:
                *componentType = createWithImplementation<IComponentType, ServerTypeImpl>(builderPtr).detach();
                return OPENDAQ_SUCCESS;
            case ComponentTypeSort::Device:
                *componentType = createWithImplementation<IComponentType, DeviceTypeImpl>(builderPtr).detach();
                return OPENDAQ_SUCCESS;
            case ComponentTypeSort::FunctionBlock:
                *componentType = createWithImplementation<IComponentType, FunctionBlockTypeImpl>(builderPtr).detach();
                return OPENDAQ_SUCCESS;
            case ComponentTypeSort::Streaming:
                *componentType = createWithImplementation<IComponentType, StreamingTypeImpl>(builderPtr).detach();
                return OPENDAQ_SUCCESS;
            case ComponentTypeSort::Undefined:
                break;
        }

        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode ComponentTypeBuilderImpl::setId(IString* id)
{
    this->id = id;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setTypeSort(ComponentTypeSort sort)
{
    this->sort = sort;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getTypeSort(ComponentTypeSort* sort)
{
    OPENDAQ_PARAM_NOT_NULL(sort);

    *sort = this->sort;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setDescription(IString* description)
{
    this->description = description;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setConnectionStringPrefix(IString* prefix)
{
    this->prefix = prefix;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getConnectionStringPrefix(IString** prefix)
{
    OPENDAQ_PARAM_NOT_NULL(prefix);

    *prefix = this->prefix.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setDefaultConfig(IPropertyObject* defaultConfig)
{
    this->defaultConfig = defaultConfig;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getDefaultConfig(IPropertyObject** defaultConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultConfig);

    *defaultConfig = this->defaultConfig.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setDefaultAuthenticationConfig(IAuthenticationConfig* defaultAuthenticationConfig)
{
    this->defaultAuthenticationConfig = defaultAuthenticationConfig;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getDefaultAuthenticationConfig(IAuthenticationConfig** defaultAuthenticationConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultAuthenticationConfig);

    *defaultAuthenticationConfig = this->defaultAuthenticationConfig.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::addSupportedPayload(IString* id, ICredentialPayloadDescriptor* payload)
{
    OPENDAQ_PARAM_NOT_NULL(id);
    OPENDAQ_PARAM_NOT_NULL(payload);

    return supportedPayloads->set(id, payload);
}

ErrCode ComponentTypeBuilderImpl::getSupportedPayloads(IDict** payloads)
{
    OPENDAQ_PARAM_NOT_NULL(payloads);

    *payloads = supportedPayloads.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::validateAuthenticationCapabilities()
{
    const bool hasSupportedPayloads = supportedPayloads.assigned() && supportedPayloads.getCount() > 0;

    if (!hasSupportedPayloads)
    {
        if (defaultAuthenticationConfig.assigned())
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDPARAMETER,
                "A default authentication config is set, but no supported credential payloads are configured");

        return OPENDAQ_SUCCESS;
    }

    if (!defaultAuthenticationConfig.assigned())
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_INVALIDPARAMETER,
            "Supported credential payloads are configured, but no default authentication config is set");

    const StringPtr defaultPayloadId = defaultAuthenticationConfig.getCredentialPayloadId();
    if (!defaultPayloadId.assigned() || !supportedPayloads.hasKey(defaultPayloadId))
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_INVALIDPARAMETER,
            "The default authentication config's payload id does not match any of the supported credential payloads");

    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createComponentTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::Undefined);
}

extern "C"
ErrCode PUBLIC_EXPORT createDeviceTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::Device);
}

extern "C"
ErrCode PUBLIC_EXPORT createStreamingTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::Streaming);
}

extern "C"
ErrCode PUBLIC_EXPORT createServerTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::Server);
}

extern "C"
ErrCode PUBLIC_EXPORT createFunctionBlockTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::FunctionBlock);
}

#endif

END_NAMESPACE_OPENDAQ
