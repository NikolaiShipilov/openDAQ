#include <opendaq/userpass_credential_payload_impl.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

UserPasswordCredentialPayloadImpl::UserPasswordCredentialPayloadImpl(const FunctionPtr& getUserPassCb)
    : getUsernameAndPasswordCallback(getUserPassCb)
{
    if (!getUsernameAndPasswordCallback.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Function callback to obtain username and password must be assigned on corresponding credential payload creation");
}

ErrCode UserPasswordCredentialPayloadImpl::getSecrets(IDict** secrets)
{
    OPENDAQ_PARAM_NOT_NULL(secrets);

    return daqTry([&]
    {
        DictPtr<IString, IBaseObject> userNameAndPassword;
        userNameAndPassword = getUsernameAndPasswordCallback();

        *secrets = userNameAndPassword.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, UserPasswordCredentialPayload, ICredentialPayload, IFunction*, getUserPassCb)

END_NAMESPACE_OPENDAQ
