#include <opendaq/cmd_line_credential_provider_impl.h>
#include <opendaq/credential_payload_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <fmt/format.h>
#include <iostream>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

static const std::string CmdLineCredentialProviderName = "CmdLineCredentialProvider";
static const std::string HideSecretInputMetaDataKey = "HideSecretInput";

CmdLineCredentialProviderImpl::CmdLineCredentialProviderImpl()
{
}

ErrCode CmdLineCredentialProviderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = String(CmdLineCredentialProviderName).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::getSupportedPayloadFormats(IList** formats)
{
    OPENDAQ_PARAM_NOT_NULL(formats);

    auto supportedFormats = List<IInteger>();
    supportedFormats.pushBack(static_cast<Int>(CredentialPayloadFormat::KeyValuePairs));
    supportedFormats.pushBack(static_cast<Int>(CredentialPayloadFormat::String));

    *formats = supportedFormats.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::requestCredentials(ICredentialRequest* request, ICredentialPayload** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(credentials);
    OPENDAQ_PARAM_NOT_NULL(request);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    const auto descriptor = requestPtr.getPayloadDescriptor();
    if (!descriptor.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Credential request has no payload descriptor set");

    const bool hide = ShouldHideSecretInput(requestPtr);

    switch (descriptor.getFormat())
    {
        case CredentialPayloadFormat::KeyValuePairs:
        {
            auto callback = Function(
                [requestPtr, hide]()
                {
                    printRequestDetails(requestPtr);
                    return readUserNameAndPassword(hide);
                });

            *credentials = UserPasswordCredentialPayload(callback).detach();
            return OPENDAQ_SUCCESS;
        }
        case CredentialPayloadFormat::String:
        {
            auto callback = Function(
                [requestPtr, descriptor, hide]()
                {
                    printRequestDetails(requestPtr);
                    return readStringSecret(descriptor, hide);
                });

            *credentials = StringCredentialPayload(callback).detach();
            return OPENDAQ_SUCCESS;
        }
        default:
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Unsupported credential payload format");
    }
}

bool CmdLineCredentialProviderImpl::ShouldHideSecretInput(const CredentialRequestPtr& request)
{
    const auto metaData = request.getMetaData();
    if (metaData.assigned() && metaData.hasProperty(HideSecretInputMetaDataKey))
        return metaData.getPropertyValue(HideSecretInputMetaDataKey);

    return true;
}

DictPtr<IString, IBaseObject> CmdLineCredentialProviderImpl::readUserNameAndPassword(bool hide)
{
    std::string username;
    std::cout << "UserName: ";
    std::getline(std::cin, username);
    if (!std::cin)
        throw std::runtime_error("Credential prompt cancelled");

    auto password = readLine("Password: ", hide);

    auto secrets = Dict<IString, IString>();
    secrets.set("UserName", String(username));
    secrets.set("Password", String(password));
    return secrets;
}

StringPtr CmdLineCredentialProviderImpl::readStringSecret(const CredentialPayloadDescriptorPtr& descriptor, bool hide)
{
    const StringPtr description = descriptor.getDescription();

    auto secret = readLine(fmt::format("{}: ", description.assigned() ? description.toStdString() : "Secret"), hide);
    return String(secret);
}

void CmdLineCredentialProviderImpl::printRequestDetails(const CredentialRequestPtr& request)
{
    std::cout << '\n';
    std::cout << "============================================================\n";
    std::cout << "Authentication required\n";
    std::cout << "============================================================\n\n";

    if (const auto connectionString = request.getConnectionString(); connectionString.assigned() && connectionString.getLength() > 0)
        std::cout << "Connection string : " << connectionString << '\n';

    if (const auto metaData = request.getMetaData(); metaData.assigned())
    {
        for (const auto& property : metaData.getAllProperties())
            std::cout << property.getDescription() << " (" << property.getName() << ") : " << metaData.getPropertyValue(property.getName()) << '\n';
    }

    std::cout << '\n';
}

std::string CmdLineCredentialProviderImpl::readLine(const std::string& prompt, bool hide)
{
    std::cout << prompt;
    std::cout.flush();

    if (!hide)
    {
        std::string value;
        if (!std::getline(std::cin, value))
            throw std::runtime_error("Input cancelled");

        return value;
    }

#ifdef _WIN32

    std::wstring value;

    while (true)
    {
        const wchar_t ch = _getwch();

        switch (ch)
        {
            case L'\r': // Enter
                std::wcout << std::endl;
                return StringConverter::Utf16ToUtf8(value);

            case 3: // Ctrl+C
                throw std::runtime_error("Input cancelled.");

            case L'\b': // Backspace
                if (!value.empty())
                    value.pop_back();
                break;

            case 0:
            case 0xE0:
                _getwch(); // Consume extended key code.
                break;

            default:
                value.push_back(ch);
                break;
        }
    }

#else

    termios oldAttr{};
    if (tcgetattr(STDIN_FILENO, &oldAttr) != 0)
        throw std::runtime_error("Failed to access terminal.");

    struct EchoGuard
    {
        explicit EchoGuard(const termios& attr)
            : oldAttr(attr)
        {
        }

        ~EchoGuard()
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldAttr);
        }

        termios oldAttr;
    } guard(oldAttr);

    termios newAttr = oldAttr;
    newAttr.c_lflag &= ~ECHO;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newAttr) != 0)
        throw std::runtime_error("Failed to disable terminal echo.");

    std::string value;

    if (!std::getline(std::cin, value))
        throw std::runtime_error("Input cancelled.");

    std::cout << std::endl;

    return value;

#endif
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CmdLineCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
