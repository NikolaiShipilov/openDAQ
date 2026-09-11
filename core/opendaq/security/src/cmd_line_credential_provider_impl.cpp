#include <opendaq/cmd_line_credential_provider_impl.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <fmt/format.h>
#include <iostream>

#ifdef _WIN32
#include <conio.h>
#include <codecvt>
#include <locale>
#else
#include <termios.h>
#include <unistd.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

static const std::string CmdLineCredentialProviderId = "CmdLineCredentialProvider";

CmdLineCredentialProviderImpl::CmdLineCredentialProviderImpl()
{
}

ErrCode CmdLineCredentialProviderImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = String(CmdLineCredentialProviderId).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::getSupportedPayloadFormats(IList** formats)
{
    OPENDAQ_PARAM_NOT_NULL(formats);

    auto supportedFormats = List<IInteger>();
    supportedFormats.pushBack(static_cast<Int>(CredentialPayloadFormat::KeyValuePairs));
    supportedFormats.pushBack(static_cast<Int>(CredentialPayloadFormat::String));
    supportedFormats.pushBack(static_cast<Int>(CredentialPayloadFormat::FilePath));

    *formats = supportedFormats.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::requestCredentials(ICredentialRequest* request, IPropertyObject** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(credentials);
    OPENDAQ_PARAM_NOT_NULL(request);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    const auto descriptor = requestPtr.getPayloadDescriptor();
    if (!descriptor.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Credential request has no payload descriptor set");

    switch (descriptor.getFormat())
    {
        case CredentialPayloadFormat::KeyValuePairs:
        {
            printRequestDetails(requestPtr);
            *credentials = readKeyValuePairs(descriptor).detach();
            return OPENDAQ_SUCCESS;
        }
        case CredentialPayloadFormat::String:
        {
            printRequestDetails(requestPtr);
            *credentials = readStringSecret(descriptor).detach();
            return OPENDAQ_SUCCESS;
        }
        case CredentialPayloadFormat::FilePath:
        {
            *credentials = readFilePathSecretCached(requestPtr, descriptor).detach();
            return OPENDAQ_SUCCESS;
        }
        default:
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Unsupported credential payload format");
    }
}

ErrCode CmdLineCredentialProviderImpl::cacheCredentials(ICredentialRequest* request, IPropertyObject* secret)
{
    OPENDAQ_PARAM_NOT_NULL(request);
    OPENDAQ_PARAM_NOT_NULL(secret);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    const auto descriptor = requestPtr.getPayloadDescriptor();
    if (!descriptor.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Credential request has no payload descriptor set");

    // Only FilePath secrets are cached (see `readFilePathSecretCached`) - other formats are a no-op here,
    // since this provider never caches them either when obtaining them interactively.
    if (descriptor.getFormat() == CredentialPayloadFormat::FilePath)
    {
        const auto secretObj = PropertyObjectPtr::Borrow(secret);
        const StringPtr propertyName = descriptor.createDefaultPayload().getAllProperties()[0].getName();
        if (!secretObj.hasProperty(propertyName))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Provided secret is not shaped like a FilePath-format payload");

        const StringPtr path = secretObj.getPropertyValue(propertyName);
        filePathSecretCache[MakeFilePathCacheKey(requestPtr)] = path.assigned() ? path.toStdString() : std::string();
    }

    return OPENDAQ_SUCCESS;
}

PropertyObjectPtr CmdLineCredentialProviderImpl::readKeyValuePairs(const CredentialPayloadDescriptorPtr& descriptor)
{
    const DictPtr<IString, IBoolean> keys = descriptor.getParameters().get("Keys");

    auto payload = descriptor.createDefaultPayload();
    for (const auto& [key, hidden] : keys)
        payload.setPropertyValue(key, String(readLine(fmt::format("{}: ", key.toStdString()), hidden)));

    return payload;
}

PropertyObjectPtr CmdLineCredentialProviderImpl::readStringSecret(const CredentialPayloadDescriptorPtr& descriptor)
{
    const StringPtr description = descriptor.getDescription();
    const auto parameters = descriptor.getParameters();
    const bool hidden = parameters.assigned() && parameters.hasField("Hidden") && (bool) parameters.get("Hidden");

    auto secret = readLine(fmt::format("{}: ", description.assigned() ? description.toStdString() : "Secret"), hidden);

    auto payload = descriptor.createDefaultPayload();
    payload.setPropertyValue(payload.getAllProperties()[0].getName(), String(secret));
    return payload;
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

PropertyObjectPtr CmdLineCredentialProviderImpl::readFilePathSecretCached(const CredentialRequestPtr& request, const CredentialPayloadDescriptorPtr& descriptor)
{
    const auto cacheKey = MakeFilePathCacheKey(request);

    if (const auto it = filePathSecretCache.find(cacheKey); it != filePathSecretCache.end())
    {
        auto payload = descriptor.createDefaultPayload();
        payload.setPropertyValue(payload.getAllProperties()[0].getName(), String(it->second));
        return payload;
    }

    printRequestDetails(request);
    const auto payload = readStringSecret(descriptor);
    const StringPtr secret = payload.getPropertyValue(payload.getAllProperties()[0].getName());
    filePathSecretCache[cacheKey] = secret.toStdString();
    return payload;
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
