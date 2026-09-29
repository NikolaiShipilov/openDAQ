#include <opendaq/cmd_line_credential_provider_impl.h>
#include <coreobjects/exceptions.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <fmt/format.h>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <conio.h>
#include <codecvt>
#include <locale>
#else
#include <termios.h>
#include <unistd.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

static const std::string CmdLineCredentialProviderDescription = "Prompts for credentials interactively via the command line.";
static constexpr int MaxFilePathAttempts = 3;

CmdLineCredentialProviderImpl::CmdLineCredentialProviderImpl()
{
}

ErrCode CmdLineCredentialProviderImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = String(CmdLineCredentialProviderDescription).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::getSupportedFormats(IList** formats)
{
    OPENDAQ_PARAM_NOT_NULL(formats);

    auto supportedFormats = List<IInteger>();
    supportedFormats.pushBack(static_cast<Int>(CredentialFormat::KeyValuePairs));
    supportedFormats.pushBack(static_cast<Int>(CredentialFormat::String));
    supportedFormats.pushBack(static_cast<Int>(CredentialFormat::FilePath));

    *formats = supportedFormats.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::requestCredentials(ICredentialRequest* request, IPropertyObject** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(credentials);
    OPENDAQ_PARAM_NOT_NULL(request);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    const auto authenticationMethod = requestPtr.getAuthenticationMethod();
    if (!authenticationMethod.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Credential request has no authentication method set");

    switch (authenticationMethod.getFormat())
    {
        case CredentialFormat::KeyValuePairs:
        {
            printRequestDetails(requestPtr);
            *credentials = readKeyValuePairs(authenticationMethod).detach();
            return OPENDAQ_SUCCESS;
        }
        case CredentialFormat::String:
        {
            printRequestDetails(requestPtr);
            *credentials = readStringCredential(authenticationMethod).detach();
            return OPENDAQ_SUCCESS;
        }
        case CredentialFormat::FilePath:
        {
            *credentials = readFilePathCredentialCached(requestPtr, authenticationMethod).detach();
            return OPENDAQ_SUCCESS;
        }
        default:
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Unsupported credential format");
    }
}

ErrCode CmdLineCredentialProviderImpl::cacheCredentials(ICredentialRequest* request, IPropertyObject* credential)
{
    OPENDAQ_PARAM_NOT_NULL(request);
    OPENDAQ_PARAM_NOT_NULL(credential);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    const auto authenticationMethod = requestPtr.getAuthenticationMethod();
    if (!authenticationMethod.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Credential request has no authentication method set");

    // Only FilePath credentials are cached (see `readFilePathCredentialCached`) - other formats are a no-op here,
    // since this provider never caches them either when obtaining them interactively.
    if (authenticationMethod.getFormat() == CredentialFormat::FilePath)
    {
        const auto credentialObj = PropertyObjectPtr::Borrow(credential);
        const StringPtr propertyName = authenticationMethod.createEmptyCredential().getAllProperties()[0].getName();
        if (!credentialObj.hasProperty(propertyName))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Provided credential is not shaped like a FilePath-format credential");

        const StringPtr path = credentialObj.getPropertyValue(propertyName);
        filePathCredentialCache[MakeFilePathCacheKey(requestPtr)] = path.assigned() ? path.toStdString() : std::string();
    }

    return OPENDAQ_SUCCESS;
}

PropertyObjectPtr CmdLineCredentialProviderImpl::readKeyValuePairs(const AuthenticationMethodPtr& authenticationMethod)
{
    const DictPtr<IString, IBoolean> keys = authenticationMethod.getParameters().get("Keys");

    auto credential = authenticationMethod.createEmptyCredential();
    for (const auto& [key, hidden] : keys)
        credential.setPropertyValue(key, String(readLine(fmt::format("{}: ", key.toStdString()), hidden)));

    return credential;
}

PropertyObjectPtr CmdLineCredentialProviderImpl::readStringCredential(const AuthenticationMethodPtr& authenticationMethod)
{
    const StringPtr description = authenticationMethod.getDescription();
    const auto parameters = authenticationMethod.getParameters();
    const bool hidden = parameters.assigned() && parameters.hasField("Hidden") && (bool) parameters.get("Hidden");

    auto credentialValue = readLine(fmt::format("{}: ", description.assigned() ? description.toStdString() : "Credential"), hidden);

    auto credential = authenticationMethod.createEmptyCredential();
    credential.setPropertyValue(credential.getAllProperties()[0].getName(), String(credentialValue));
    return credential;
}

CmdLineCredentialProviderImpl::CacheKey CmdLineCredentialProviderImpl::MakeFilePathCacheKey(const CredentialRequestPtr& request)
{
    const StringPtr manufacturer = request.getManufacturer();
    const StringPtr serialNumber = request.getSerialNumber();
    const bool hasManufacturer = manufacturer.assigned() && manufacturer.getLength() > 0;
    const bool hasSerialNumber = serialNumber.assigned() && serialNumber.getLength() > 0;

    if (!hasManufacturer && !hasSerialNumber)
    {
        // Neither is available to identify the connection by - e.g. a streaming connection, which (unlike
        // a device's `daq://manufacturer_serial` smart string) isn't resolved through manufacturer/serial
        // discovery. Fall back to the connection string itself - the module that formed this request is
        // expected to have already canonicalized it (routing prefix trimmed, every parameter made explicit;
        // see `Module::onGetCanonicalConnectionString`), so it stays a stable identifier regardless of how
        // much of it the caller originally left implicit.
        const StringPtr connectionString = request.getConnectionString();
        return std::make_pair(connectionString.assigned() ? connectionString.toStdString() : std::string(), std::string());
    }

    return std::make_pair(hasManufacturer ? manufacturer.toStdString() : std::string(),
                          hasSerialNumber ? serialNumber.toStdString() : std::string());
}

PropertyObjectPtr CmdLineCredentialProviderImpl::readFilePathCredentialCached(const CredentialRequestPtr& request, const AuthenticationMethodPtr& authenticationMethod)
{
    const auto cacheKey = MakeFilePathCacheKey(request);

    if (const auto it = filePathCredentialCache.find(cacheKey); it != filePathCredentialCache.end())
    {
        auto credential = authenticationMethod.createEmptyCredential();
        credential.setPropertyValue(credential.getAllProperties()[0].getName(), String(it->second));
        return credential;
    }

    printRequestDetails(request);
    const auto credential = readFilePathCredential(authenticationMethod);
    const StringPtr credentialValue = credential.getPropertyValue(credential.getAllProperties()[0].getName());
    filePathCredentialCache[cacheKey] = credentialValue.toStdString();
    return credential;
}

PropertyObjectPtr CmdLineCredentialProviderImpl::readFilePathCredential(const AuthenticationMethodPtr& authenticationMethod)
{
    const StringPtr description = authenticationMethod.getDescription();
    const std::string prompt = fmt::format("{}: ", description.assigned() ? description.toStdString() : "File path");

    for (int attempt = 1; attempt <= MaxFilePathAttempts; ++attempt)
    {
        const std::string value = readLine(prompt, false);

        if (isFileAccessible(value))
        {
            auto credential = authenticationMethod.createEmptyCredential();
            credential.setPropertyValue(credential.getAllProperties()[0].getName(), String(value));
            return credential;
        }

        const int attemptsLeft = MaxFilePathAttempts - attempt;
        std::cout << "File \"" << value << "\" does not exist or is not accessible.";
        if (attemptsLeft > 0)
            std::cout << " " << attemptsLeft << " attempt(s) left.\n";
        else
            std::cout << '\n';
    }

    DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                         "Credential provider could not obtain an accessible file path after {} attempts", MaxFilePathAttempts);
}

bool CmdLineCredentialProviderImpl::isFileAccessible(const std::string& path)
{
    return std::ifstream(path).good();
}

void CmdLineCredentialProviderImpl::printRequestDetails(const CredentialRequestPtr& request)
{
    std::cout << '\n';
    std::cout << "============================================================\n";
    std::cout << "Authentication required\n";
    std::cout << "============================================================\n\n";

    std::cout << "Component type : " << request.getComponentType().getName() << '\n';
    std::cout << "Connection string : " << request.getConnectionString() << '\n';

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
            {
                std::wcout << std::endl;
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
                return converter.to_bytes(value);
            }

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
