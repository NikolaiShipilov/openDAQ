#include <opendaq/cmd_line_credential_provider_impl.h>
#include <opendaq/credential_payload_factory.h>
#include <coretypes/listobject_factory.h>
#include <iostream>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

static const std::string CmdLineCredentialProviderName = "CmdLineCredentialProvider";

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

    *formats = supportedFormats.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::requestCredentials(ICredentialRequest* request, ICredentialPayload** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(credentials);
    OPENDAQ_PARAM_NOT_NULL(request);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    auto callback = Function(
        [requestPtr]()
        {
            printRequestDetails(requestPtr);

            std::string username;
            {
                std::cout << "UserName: ";
                std::getline(std::cin, username);
                if (!std::cin)
                    throw std::runtime_error("Credential prompt cancelled");
            }
            auto password = readPassword("Password: ");

            auto secret = Dict<IString, IString>();
            secret.set("UserName", String(username));
            secret.set("Password", String(password));

            return secret;
        });

    *credentials = UserPasswordCredentialPayload(callback).detach();
    return OPENDAQ_SUCCESS;
}

void CmdLineCredentialProviderImpl::printRequestDetails(const CredentialRequestPtr& request)
{
    std::cout << '\n';
    std::cout << "============================================================\n";
    std::cout << "Authentication required\n";
    std::cout << "============================================================\n\n";

    if (const auto type = request.getComponentType(); type.assigned())
        std::cout << "Component type : " << type.getName() << '\n';


    if (const auto connectionString = request.getConnectionString(); connectionString.assigned() && connectionString.getLength() > 0)
        std::cout << "Connection string : " << connectionString << '\n';

    std::cout << '\n';
}

std::string CmdLineCredentialProviderImpl::readPassword(const std::string &prompt)
{
    std::cout << prompt;
    std::cout.flush();

#ifdef _WIN32

    std::wstring password;

    while (true)
    {
        const wchar_t ch = _getwch();

        switch (ch)
        {
            case L'\r': // Enter
                std::wcout << std::endl;
                return StringConverter::Utf16ToUtf8(password);

            case 3: // Ctrl+C
                throw std::runtime_error("Password entry cancelled.");

            case L'\b': // Backspace
                if (!password.empty())
                    password.pop_back();
                break;

            case 0:
            case 0xE0:
                _getwch(); // Consume extended key code.
                break;

            default:
                password.push_back(ch);
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

    std::string password;

    if (!std::getline(std::cin, password))
        throw std::runtime_error("Password entry cancelled.");

    std::cout << std::endl;

    return password;

#endif
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CmdLineCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
