#include <opendaq/credential_payload_descriptor_impl.h>
#include <opendaq/credential_payload_descriptor_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    inline StructTypePtr RequireRegisteredType(const TypeManagerPtr& typeManager, const StringPtr& typeName)
    {
        if (!typeManager.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type manager must be assigned when creating a \"{}\"", typeName);
        if (!typeManager.hasType(typeName))
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type \"{}\" is not registered in the type manager", typeName);
        return typeManager.getType(typeName);
    }

    inline StringPtr RequireRegisteredClassName(const TypeManagerPtr& typeManager, const StringPtr& className)
    {
        if (!typeManager.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type manager must be assigned when creating a credential payload descriptor");
        if (!className.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Payload class name must be assigned");
        if (!typeManager.hasType(className))
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Type \"{}\" is not registered in the type manager", className);
        return className;
    }
}

//
// CredentialPayloadDescriptorParametersImpl
//

CredentialPayloadDescriptorParametersImpl::CredentialPayloadDescriptorParametersImpl(const StructTypePtr& structType,
                                                                                     const DictPtr<IString, IBaseObject>& fields)
    : GenericStructImpl<IStruct>(structType, fields)
{
}

//
// CredentialPayloadDescriptorImpl
//

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(
    const StringPtr& id,
    const DictPtr<IString, IBoolean>& keys,
    const StringPtr& description,
    const StructTypePtr& parametersType)
{
    if (!keys.assigned() || keys.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned and non-empty when creating a key-value credential payload descriptor");

    const auto parameters =
        createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(parametersType, Dict<IString, IBaseObject>({{"Keys", keys}}));

    return Dict<IString, IBaseObject>({{"Id", id}, {"Description", description}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(
    const StringPtr& id,
    const StringPtr& description,
    Bool hidden,
    const StructTypePtr& parametersType)
{
    const auto parameters = createWithImplementation<IStruct, CredentialPayloadDescriptorParametersImpl>(
        parametersType, Dict<IString, IBaseObject>({{"Hidden", hidden}}));

    return Dict<IString, IBaseObject>({{"Id", id}, {"Description", description}, {"Parameters", parameters}});
}

DictPtr<IString, IBaseObject> CredentialPayloadDescriptorImpl::BuildFields(const StringPtr& id, const StringPtr& description)
{
    return Dict<IString, IBaseObject>({{"Id", id}, {"Description", description}});
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(
    const StringPtr& id,
    const DictPtr<IString, IBoolean>& keys,
    const StringPtr& description,
    const TypeManagerPtr& typeManager,
    const StringPtr& payloadClassName)
    : CredentialPayloadDescriptorImpl(
          CredentialPayloadFormat::KeyValuePairs,
          detail::RequireRegisteredType(typeManager, KeyValuePayloadDescriptorStructType().getName()),
          BuildFields(id, keys, description, detail::RequireRegisteredType(typeManager, KeyValuePayloadDescriptorParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, payloadClassName))
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(
    const StringPtr& id, const StringPtr& description, Bool hidden, const TypeManagerPtr& typeManager, const StringPtr& payloadClassName)
    : CredentialPayloadDescriptorImpl(
          CredentialPayloadFormat::String,
          detail::RequireRegisteredType(typeManager, StringPayloadDescriptorStructType().getName()),
          BuildFields(id, description, hidden, detail::RequireRegisteredType(typeManager, StringPayloadDescriptorParametersStructType().getName())),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, payloadClassName))
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(
    const StringPtr& id, const StringPtr& description, const TypeManagerPtr& typeManager, const StringPtr& payloadClassName)
    : CredentialPayloadDescriptorImpl(
          CredentialPayloadFormat::FilePath,
          detail::RequireRegisteredType(typeManager, FilePathPayloadDescriptorStructType().getName()),
          BuildFields(id, description),
          typeManager,
          detail::RequireRegisteredClassName(typeManager, payloadClassName))
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(const StringPtr& id, const StringPtr& description, const TypeManagerPtr& typeManager)
    : CredentialPayloadDescriptorImpl(
          CredentialPayloadFormat::None,
          detail::RequireRegisteredType(typeManager, NonePayloadDescriptorStructType().getName()),
          BuildFields(id, description),
          typeManager,
          nullptr)
{
}

CredentialPayloadDescriptorImpl::CredentialPayloadDescriptorImpl(CredentialPayloadFormat format,
                                                                 const StructTypePtr& structType,
                                                                 const DictPtr<IString, IBaseObject>& fields,
                                                                 const TypeManagerPtr& typeManager,
                                                                 const StringPtr& payloadClassName)
    : GenericStructImpl<ICredentialPayloadDescriptor, IStruct>(structType, fields)
    , format(format)
    , typeManager(typeManager)
    , payloadClassName(payloadClassName)
{
}

ErrCode CredentialPayloadDescriptorImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->fields.get("Id").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::getFormat(CredentialPayloadFormat* formatOut)
{
    OPENDAQ_PARAM_NOT_NULL(formatOut);

    *formatOut = this->format;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::getParameters(IStruct** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    if (!this->fields.hasKey("Parameters"))
    {
        *parameters = nullptr;
        return OPENDAQ_SUCCESS;
    }

    *parameters = this->fields.get("Parameters").template asPtr<IStruct>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->fields.get("Description").template asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorImpl::createDefaultPayload(IPropertyObject** payload)
{
    OPENDAQ_PARAM_NOT_NULL(payload);

    if (format == CredentialPayloadFormat::None)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The \"None\" format requires no credentials, so it has no payload to build");

    *payload = PropertyObject(typeManager, payloadClassName).detach();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IDict*, keys, IString*, description, ITypeManager*, typeManager, IString*, payloadClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringPayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description, Bool, hidden, ITypeManager*, typeManager, IString*, payloadClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FilePathPayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description, ITypeManager*, typeManager, IString*, payloadClassName)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NonePayloadDescriptor, ICredentialPayloadDescriptor, IString*, id, IString*, description, ITypeManager*, typeManager)

END_NAMESPACE_OPENDAQ
