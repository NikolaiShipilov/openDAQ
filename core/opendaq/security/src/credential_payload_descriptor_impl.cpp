#include <opendaq/credential_payload_descriptor_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

CredentialPayloadDescriptorBaseImpl::CredentialPayloadDescriptorBaseImpl(const PropertyObjectPtr& parameters, const StringPtr& description)
    : parameters(parameters)
    , description(description)
{
}

ErrCode CredentialPayloadDescriptorBaseImpl::getParameters(IPropertyObject** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->parameters.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialPayloadDescriptorBaseImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

KeyValuePayloadDescriptorImpl::KeyValuePayloadDescriptorImpl(const ListPtr<IString>& keys, const StringPtr& description)
    : CredentialPayloadDescriptorBaseImpl(BuildParameters(keys), description)
{
}

PropertyObjectPtr KeyValuePayloadDescriptorImpl::BuildParameters(const ListPtr<IString>& keys)
{
    if (!keys.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Keys must be assigned when creating a key-value credential payload descriptor");

    auto params = PropertyObject();
    params.addProperty(ListProperty("Keys", keys));
    return params;
}

ErrCode KeyValuePayloadDescriptorImpl::getFormat(CredentialPayloadFormat* format)
{
    OPENDAQ_PARAM_NOT_NULL(format);

    *format = CredentialPayloadFormat::KeyValuePairs;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor, IList*, keys, IString*, description)

END_NAMESPACE_OPENDAQ
