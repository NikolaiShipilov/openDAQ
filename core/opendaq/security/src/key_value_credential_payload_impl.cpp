#include <opendaq/key_value_credential_payload_impl.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

KeyValueCredentialPayloadImpl::KeyValueCredentialPayloadImpl(const FunctionPtr& getValuesCb)
    : getValuesCallback(getValuesCb)
{
    if (!getValuesCallback.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Function callback to obtain key-value credential secrets must be assigned on corresponding credential payload creation");
}

ErrCode KeyValueCredentialPayloadImpl::getSecrets(IBaseObject** secrets)
{
    OPENDAQ_PARAM_NOT_NULL(secrets);

    return daqTry([&]
    {
        const BaseObjectPtr result = getValuesCallback();

        auto values = result.asPtrOrNull<IDict, DictPtr<IString, IString>>();
        if (!values.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Credential provider's callback did not return a key-value dictionary");

        *secrets = values.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValueCredentialPayload, ICredentialPayload, IFunction*, getValuesCb)

END_NAMESPACE_OPENDAQ
