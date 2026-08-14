#include <opendaq/component_type_builder_impl.h>
#include <opendaq/server_type_impl.h>
#include <opendaq/device_type_impl.h>
#include <opendaq/function_block_type_impl.h>
#include <opendaq/streaming_type_impl.h>
#include <coretypes/validation.h>
#include <coretypes/dictobject_factory.h>
#include <opendaq/authentication_config_factory.h>

BEGIN_NAMESPACE_OPENDAQ

ComponentTypeBuilderImpl::ComponentTypeBuilderImpl(ComponentTypeSort sort)
    : sort(sort)
    , supportedAuthenticationConfigs(Dict<IString, IAuthenticationConfig>())
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

ErrCode ComponentTypeBuilderImpl::setDefaultAuthenticationConfigId(IString* id)
{
    this->defaultAuthenticationConfigId = id;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getDefaultAuthenticationConfigId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = defaultAuthenticationConfigId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::addSupportedAuthenticationConfig(IString* id, ICredentialPayloadDescriptor* payloadDescriptor, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(id);
    OPENDAQ_PARAM_NOT_NULL(payloadDescriptor);

    supportedAuthenticationConfigs.set(id, AuthenticationConfig(id, payloadDescriptor, config));
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getSupportedAuthenticationConfigs(IDict** authenticationConfigs)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationConfigs);

    *authenticationConfigs = supportedAuthenticationConfigs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::validateAuthenticationCapabilities()
{
    const bool noneAuthenticationConfigs = !supportedAuthenticationConfigs.assigned() || supportedAuthenticationConfigs.getCount() == 0;

    if (noneAuthenticationConfigs)
    {
        if (defaultAuthenticationConfigId.assigned())
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDPARAMETER,
                "A default authentication config id is set, but no supported authentication configs were added");

        return OPENDAQ_SUCCESS;
    }
    else
    {
        if (!defaultAuthenticationConfigId.assigned())
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDPARAMETER,
                "Supported authentication configs were added, but no default authentication config id is set");

        if (!supportedAuthenticationConfigs.hasKey(defaultAuthenticationConfigId))
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDPARAMETER,
                "The default authentication config id does not match any of the supported authentication configs");
    }

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
