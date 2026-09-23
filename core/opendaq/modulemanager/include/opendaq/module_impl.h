/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <opendaq/module.h>
#include <opendaq/device_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/context_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/server_capability_config_ptr.h>
#include <opendaq/custom_log.h>
#include <opendaq/module_info_factory.h>
#include <opendaq/component_type_private.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/authentication_config_factory.h>
#include <opendaq/credential_descriptor_ptr.h>
#include <opendaq/credential_provider_ptr.h>
#include <opendaq/credential_request_ptr.h>
#include <opendaq/credential_request_factory.h>
#include <coreobjects/exceptions.h>
#include <opendaq/component_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class Module : public ImplementationOf<IModule>
{
public:

    /*!
     * @brief Retrieves the module information.
     * @param[out] info The module information.
     */
    ErrCode INTERFACE_FUNC getModuleInfo(IModuleInfo** info) override
    {
        OPENDAQ_PARAM_NOT_NULL(info);

        *info = moduleInfo.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Returns a list of known devices info.
     * The implementation can start discovery in background and only return the results in this function.
     * @param[out] availableDevices The list of known devices information.
     */
    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override
    {
        OPENDAQ_PARAM_NOT_NULL(availableDevices);

        ListPtr<IDeviceInfo> availableDevicesPtr;
        const ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableDevices, availableDevicesPtr);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        *availableDevices = availableDevicesPtr.detach();
        return errCode;
    }

    /*!
     * @brief Returns a dictionary of known and available device types this module can create.
     * @param[out] deviceTypes The dictionary of known device types.
     */
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(deviceTypes);

        DictPtr<IString, IDeviceType> types;
        const ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableDeviceTypes, types);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        for (const auto& type : types)
        {
            auto componentTypePrivate = type.second.asPtr<IComponentTypePrivate>();
            componentTypePrivate->setModuleInfo(this->moduleInfo);
        }

        *deviceTypes = types.detach();
        return errCode;
    }

    /*!
     * @brief Creates a device object that can communicate with the device described in the specified connection string.
     * The device object is not automatically added as a sub-device of the caller, but only returned by reference.
     * @param connectionString Describes the connection info of the device to connect to. 
     * If connection string starts with `daq://`, module chooses the optimal server capability based on protocol type
     * @param parent The parent component/device to which the device attaches.
     * @param[out] device The device object created to communicate with and control the device.
     */
    ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config) override
    {
        OPENDAQ_PARAM_NOT_NULL(connectionString);
        OPENDAQ_PARAM_NOT_NULL(device);

        DictPtr<IString, IDeviceType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableDeviceTypes, types);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTIMPLEMENTED);

        ComponentTypePtr deviceType;
        const StringPtr prefix = getPrefixFromConnectionString(connectionString);
        if (prefix.assigned() && prefix.getLength() != 0)
        {
            for (const auto& [_, type] : types)
            {
                if (type.getConnectionStringPrefix() == prefix)
                {
                    deviceType = type;
                    break;
                }
            }
        }

        DevicePtr createdDevice;
        errCode = wrapHandlerReturn(this, &Module::onCreateDevice, createdDevice, connectionString, parent, mergeConfig(config, deviceType));
        OPENDAQ_RETURN_IF_FAILED(errCode);

        if (createdDevice.assigned())
            createdDevice.getInfo();

        *device = createdDevice.detach();
        return errCode;
    }

    /*!
     * @brief Creates a device object that can communicate with the device described in the specified connection string,
     * authenticating the connection by obtaining credentials - as specified by the given authentication configuration -
     * from a compatible registered credential provider.
     * The device object is not automatically added as a sub-device of the caller, but only returned by reference.
     * @param connectionString Describes the connection info of the device to connect to.
     * If connection string starts with `daq://`, module chooses the optimal server capability based on protocol type
     * @param manufacturer The manufacturer of the device to connect to.
     * @param serialNumber The serial number of the device to connect to.
     * @param parent The parent component/device to which the device attaches.
     * @param[out] device The device object created to communicate with and control the device.
     * @param authenticationConfig The authentication configuration used to authenticate the connection to the device.
     *
     * The credentials are resolved (`requestCredentials`) here before `onCreateAuthenticatedDevice` is called.
     * On success, `authenticationConfig` is persisted on the created device (`IComponentPrivate::setAuthenticationConfig`)
     * so a reload can re-request credentials for it - this, too, is handled here rather than by the module implementation.
     * `manufacturer`/`serialNumber` are used for that resolution (as metadata on the credential request/for a
     * provider's own caching).
     *
     * If either wasn't known at that point, the created device's own info can provide them afterward - if it
     * supplies both, the already-obtained credentials are handed again (`cacheCredentials`) to the exact same
     * provider `requestCredentials` resolved originally (never re-searched), against a request rebuilt with them.
     */
    ErrCode INTERFACE_FUNC createAuthenticatedDevice(IDevice** device,
                                                     IString* connectionString,
                                                     IString* manufacturer,
                                                     IString* serialNumber,
                                                     IComponent* parent,
                                                     IPropertyObject* config,
                                                     IAuthenticationConfig* authenticationConfig) override
    {
        OPENDAQ_PARAM_NOT_NULL(connectionString);
        OPENDAQ_PARAM_NOT_NULL(device);

        DictPtr<IString, IDeviceType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableDeviceTypes, types);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTIMPLEMENTED);

        ComponentTypePtr deviceType;
        const StringPtr prefix = getPrefixFromConnectionString(connectionString);
        if (prefix.assigned() && prefix.getLength() != 0)
        {
            for (const auto& [_, type] : types)
            {
                if (type.getConnectionStringPrefix() == prefix)
                {
                    deviceType = type;
                    break;
                }
            }
        }

        const AuthenticationConfigPtr authConfigPtr = AuthenticationConfigPtr::Borrow(authenticationConfig);

        CredentialProviderPtr resolvedProvider;
        PropertyObjectPtr credentials;

        errCode = wrapHandlerReturn(
            this, &Module::obtainCredentials, credentials, authConfigPtr, connectionString, manufacturer, serialNumber, deviceType, resolvedProvider);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        const StringPtr authenticationMethodId = authConfigPtr.getAuthenticationMethodId();

        DevicePtr createdDevice;
        errCode = wrapHandlerReturn(this,
                                    &Module::onCreateAuthenticatedDevice,
                                    createdDevice,
                                    connectionString,
                                    parent,
                                    mergeConfig(config, deviceType),
                                    authenticationMethodId,
                                    credentials);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        if (createdDevice.assigned())
        {
            errCode = daqTry([&]
            {
                if (const auto& componentPrivate = createdDevice.asPtrOrNull<IComponentPrivate>(true); componentPrivate.assigned())
                    componentPrivate.setAuthenticationConfig(authConfigPtr);

                const DeviceInfoPtr info = createdDevice.getInfo();

                // If manufacturer/serial number weren't known when `requestCredentials` was originally called
                // above, but the created device's own info now supplies them, hand the already-obtained
                // credentials to the same provider again, keyed by them too.
                if ((manufacturer == nullptr || serialNumber == nullptr) && resolvedProvider.assigned() && info.assigned() &&
                    info.getManufacturer().assigned() && info.getSerialNumber().assigned())
                {
                    try
                    {
                        const auto enrichedRequest = buildCredentialRequest(
                            authConfigPtr, connectionString, info.getManufacturer(), info.getSerialNumber(), deviceType);
                        resolvedProvider.cacheCredentials(enrichedRequest, credentials);
                    }
                    catch (const DaqException& e)
                    {
                        LOG_W("Failed to re-cache credentials with resolved manufacturer/serial number: {}", e.what())
                    }
                    catch (const std::exception& e)
                    {
                        LOG_W("Failed to re-cache credentials with resolved manufacturer/serial number: {}", e.what())
                    }
                }

                return OPENDAQ_SUCCESS;
            });
            OPENDAQ_RETURN_IF_FAILED(errCode);
        }

        *device = createdDevice.detach();
        return errCode;
    }

    /*!
     * @brief Returns a dictionary of known and available function blocks this module can create.
     * @param[out] functionBlockTypes The dictionary of known function blocks types.
     */
    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(functionBlockTypes);

        DictPtr<IString, IFunctionBlockType> types;
        const ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableFunctionBlockTypes, types);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    
        for (const auto& type : types)
        {
            auto componentTypePrivate = type.second.asPtr<IComponentTypePrivate>();
            componentTypePrivate->setModuleInfo(this->moduleInfo);
        }

        *functionBlockTypes = types.detach();
        return errCode;
    }

    /*!
     * @brief Creates and returns a function block with the specified id.
     * The function block is not automatically added to the FB list of the caller.
     * @param id The id of the function block to create. Ids can be retrieved by calling `getAvailableFunctionBlockTypes()`.
     * @param parent The parent component/folder/device to which the device attaches.
     * @param localId The local id of the function block.
     * @param config Function block configuration. In case of a null value, implementation should use default configuration.
     * @param[out] functionBlock The created function block.
     */
    ErrCode INTERFACE_FUNC createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, daq::IString* localId, IPropertyObject* config = nullptr) override
    {
        OPENDAQ_PARAM_NOT_NULL(id);
        OPENDAQ_PARAM_NOT_NULL(functionBlock);
        
        DictPtr<IString, IComponentType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableFunctionBlockTypes, types);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTIMPLEMENTED);

        ComponentTypePtr type;
        if (types.assigned())
            type = types.getOrDefault(id);

        FunctionBlockPtr block;
        errCode = wrapHandlerReturn(this, &Module::onCreateFunctionBlock, block, id, parent, localId, mergeConfig(config, type));
        OPENDAQ_RETURN_IF_FAILED(errCode);

        if (const auto& componentPrivate = block.asPtrOrNull<IComponentPrivate>(true); componentPrivate.assigned())
            componentPrivate.setComponentConfig(config);

        *functionBlock = block.detach();
        return errCode;
    }

    /*!
     * @brief Returns a dictionary of known and available server types this module can create.
     * @param[out] serverTypes The dictionary of known server types information.
     */
    ErrCode INTERFACE_FUNC getAvailableServerTypes(IDict** serverTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(serverTypes);

        DictPtr<IString, IServerType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableServerTypes, types);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        for (const auto& type : types)
        {
            auto componentTypePrivate = type.second.asPtr<IComponentTypePrivate>();
            componentTypePrivate->setModuleInfo(this->moduleInfo);
        }

        *serverTypes = types.detach();
        return errCode;
    }


    /*!
     * @brief Creates and returns a server with the specified serverType.
     * @param serverTypeId The id of the server to create. Ids can be retrieved by calling `getAvailableServerTypes()`.
     * @param config Server configuration. In case of a null value, implementation should use default configuration.
     * @param rootDevice Root device.
     * @param[out] server The created server.
     */
    ErrCode INTERFACE_FUNC createServer(daq::IServer** server, IString* serverTypeId, daq::IDevice* rootDevice, IPropertyObject* config) override
    {
        OPENDAQ_PARAM_NOT_NULL(serverTypeId);
        OPENDAQ_PARAM_NOT_NULL(server);

        DictPtr<IString, IComponentType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableServerTypes, types);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTIMPLEMENTED);

        ComponentTypePtr type;
        if (types.assigned())
            type = types.getOrDefault(serverTypeId);

        ServerPtr serverInstance;
        errCode = wrapHandlerReturn(this, &Module::onCreateServer, serverInstance, serverTypeId, mergeConfig(config, type), rootDevice);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        *server = serverInstance.detach();
        return errCode;
    }

    /*!
     * @brief Creates and returns a streaming object using the specified connection string and config object,
     * optionally authenticating the connection by obtaining credentials - as specified by the given authentication
     * configuration - from a compatible registered credential provider.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `daq.lt//`.
     * @param config A config object that contains parameters used to configure a streaming connection.
     * In case of a null value, implementation should use default configuration.
     * @param authenticationConfig The authentication configuration used to authenticate the streaming connection. If
     * unassigned and the resolved streaming type supports authentication, its own default config is used instead
     * - the streaming is connected to without authentication only if the type doesn't support it at all.
     * @param manufacturer The manufacturer of the device the streaming connection belongs to, if known.
     * @param serialNumber The serial number of the device the streaming connection belongs to, if known.
     * @param[out] streaming The created streaming object.
     *
     * The credentials are resolved (`requestCredentials`) before `onCreateAuthenticatedStreaming` is called.
     * If the resolved authentication method's format is `None` (e.g. `"Anonymous"`, including when
     * `authenticationConfig` is unassigned and the resolved streaming type doesn't support authentication
     * at all), no credentials are requested and `onCreateStreaming` is called instead - `authenticationConfig`
     * itself never reaches the final module's own implementation either way, only the resolved authentication
     * method id and credentials (to verify them against the right method) do, and only when they apply.
     */
    ErrCode INTERFACE_FUNC createStreaming(IStreaming** streaming,
                                           IString* connectionString,
                                           IPropertyObject* config = nullptr,
                                           IAuthenticationConfig* authenticationConfig = nullptr,
                                           IString* manufacturer = nullptr,
                                           IString* serialNumber = nullptr) override
    {
        OPENDAQ_PARAM_NOT_NULL(streaming);
        OPENDAQ_PARAM_NOT_NULL(connectionString);

        DictPtr<IString, IStreamingType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableStreamingTypes, types);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTIMPLEMENTED);

        StreamingTypePtr streamingType;
        const StringPtr prefix = getPrefixFromConnectionString(connectionString);
        if (prefix.assigned() && prefix.getLength() != 0)
        {
            for (const auto& [_, type] : types)
            {
                if (type.getConnectionStringPrefix() == prefix)
                {
                    streamingType = type;
                    break;
                }
            }
        }

        if (!streamingType.assigned())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "No streaming type matching connection string was found");
        }

        AuthenticationConfigPtr resolvedAuthConfig;
        errCode = wrapHandlerReturn(this,
                                    &Module::resolveDefaultAuthenticationConfig,
                                    resolvedAuthConfig,
                                    AuthenticationConfigPtr::Borrow(authenticationConfig),
                                    streamingType);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        const bool authenticated =
            resolvedAuthConfig.assigned() && resolvedAuthConfig.getCredentialDescriptor().getFormat() != CredentialFormat::None;

        StreamingPtr createdStreaming;
        if (authenticated)
        {
            CredentialProviderPtr resolvedProvider;
            PropertyObjectPtr credentials;
            errCode = wrapHandlerReturn(this,
                                        &Module::obtainCredentials,
                                        credentials,
                                        resolvedAuthConfig,
                                        connectionString,
                                        manufacturer,
                                        serialNumber,
                                        streamingType,
                                        resolvedProvider);
            OPENDAQ_RETURN_IF_FAILED(errCode);

            const StringPtr authenticationMethodId = resolvedAuthConfig.getAuthenticationMethodId();

            errCode = wrapHandlerReturn(this,
                                        &Module::onCreateAuthenticatedStreaming,
                                        createdStreaming,
                                        connectionString,
                                        mergeConfig(config, streamingType),
                                        authenticationMethodId,
                                        credentials);
        }
        else
        {
            errCode = wrapHandlerReturn(
                this, &Module::onCreateStreaming, createdStreaming, connectionString, mergeConfig(config, streamingType));
        }
        OPENDAQ_RETURN_IF_FAILED(errCode);

        *streaming = createdStreaming.detach();
        return errCode;
    }

    ErrCode INTERFACE_FUNC completeServerCapability(Bool* succeeded, IServerCapability* source, IServerCapabilityConfig* target) override
    {
        OPENDAQ_PARAM_NOT_NULL(target);
        OPENDAQ_PARAM_NOT_NULL(source);

        *succeeded = false;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onCompleteServerCapability, *succeeded, source, target);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        return errCode;
    }

    ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** streamingTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(streamingTypes);

        DictPtr<IString, IStreamingType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableStreamingTypes, types);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        for (const auto& type : types)
        {
            auto componentTypePrivate = type.second.asPtr<IComponentTypePrivate>();
            componentTypePrivate->setModuleInfo(this->moduleInfo);
        }

        *streamingTypes = types.detach();
        return errCode;
    }

    // Helpers

    /*!
     * @brief Retrieves information about known devices.
     * The implementation can start discovery in background and only return the results in this function.
     * @returns The list of known devices information.
     */
    virtual ListPtr<IDeviceInfo> onGetAvailableDevices()
    {
        return List<IDeviceInfo>();
    }

    /*!
     * @brief Retrieves a dictionary of known and available device types this module can create.
     * @returns A dictionary of known device types information.
     */
    virtual DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes()
    {
        return Dict<IString, IDeviceType>();
    }

    /*!
     * @brief Creates a device object that can communicate with the device described in the specified connection string.
     * The device object is not automatically added as a sub-device of the caller, but only returned by reference.
     * @param connectionString Describes the connection info of the device to connect to.
     * @param parent The parent component/device to which the device attaches.
     * @param config A configuration object that contains parameters used to configure a device in the form of key-value pairs.
     * @returns The device object created to communicate with and control the device.
     */
    virtual DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config)
    {
        return nullptr;
    }

    /*!
     * @brief Returns the canonical form of `connectionString` - the module's own connection-string prefix
     * trimmed off, and every connection parameter (port, path, etc.) made explicit, even ones the original
     * string left unspecified and the module fell back to a default for. Meant to be used as a stable
     * identifier for caching authentication credentials against a specific connection within a credential
     * provider - two connection strings that differ only in how much they left implicit still resolve
     * to the exact same canonical string, and so the same cache entry.
     * @param connectionString The connection string to canonicalize.
     * @returns The canonical connection string. The base implementation performs no canonicalization at
     * all - it returns `connectionString` unchanged - so a module that doesn't yet support authentication
     * (and so has no need for a caching identifier) doesn't need to override this.
     */
    virtual StringPtr onGetCanonicalConnectionString(const StringPtr& connectionString)
    {
        return connectionString;
    }

    /*!
     * @brief Creates a device object that can communicate with the device described in the specified connection string,
     * authenticating the connection with the given, already-resolved credentials.
     * The device object is not automatically added as a sub-device of the caller, but only returned by reference.
     * @param connectionString Describes the connection info of the device to connect to.
     * @param parent The parent component/device to which the device attaches.
     * @param config A configuration object that contains parameters used to configure a device in the form of key-value pairs.
     * @param authenticationMethodId The id of the authentication method the resolved `credentials` are shaped for (see
     * `IAuthenticationConfig::getAuthenticationMethodId`).
     * @param credentials The already-resolved credentials to authenticate with - `createAuthenticatedDevice`
     * has already obtained this (via `requestCredentials`, from a supplied secret or a credential provider)
     * before calling this method, so the implementation only needs to verify it, never to resolve it itself.
     * @returns The device object created to communicate with and control the device.
     */
    virtual DevicePtr onCreateAuthenticatedDevice(const StringPtr& connectionString,
                                                  const ComponentPtr& parent,
                                                  const PropertyObjectPtr& config,
                                                  const StringPtr& authenticationMethodId,
                                                  const PropertyObjectPtr& credentials)
    {
        return nullptr;
    }

    /*!
     * @brief Retrieves a dictionary of known and available function blocks this module can create.
     * @returns A dictionary of known function blocks information.
     */
    virtual DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes()
    {
        return Dict<IString, IFunctionBlockType>();
    }

    /*!
     * @brief Creates and returns a function block with the specified id.
     * The function block is not automatically added to the FB list of the caller.
     * @param id The id of the function block to create. Ids can be retrieved by calling `getAvailableFunctionBlockTypes()`.
     * @param parent The parent component/folder/device to which the device attaches.
     * @returns The created function block.
     */
    virtual FunctionBlockPtr onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    {
        DAQ_THROW_EXCEPTION(NotFoundException);
    }

    /*!
     * @brief Retrieves a dictionary of known and available server types this module can create.
     * @returns The dictionary of known server types information.
     */
    virtual DictPtr<IString, IServerType> onGetAvailableServerTypes()
    {
        return Dict<IString, IServerType>();
    }

    virtual DictPtr<IString, IStreamingType> onGetAvailableStreamingTypes()
    {
        return Dict<IString, IStreamingType>();
    }

    virtual ServerPtr onCreateServer(const StringPtr& serverType, const PropertyObjectPtr& serverConfig, const DevicePtr& rootDevice)
    {
        return nullptr;
    }

    /*!
     * @brief Creates and returns a streaming object using the specified connection string and config object.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `daq.lt//`.
     * @param config A config object that contains parameters used to configure a streaming connection.
     * @returns The created streaming object.
     */
    virtual StreamingPtr onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config)
    {
        return nullptr;
    }

    /*!
     * @brief Creates and returns a streaming object using the specified connection string and config object,
     * authenticating the connection with the given, already-resolved credentials.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `daq.lt//`.
     * @param config A config object that contains parameters used to configure a streaming connection.
     * @param authenticationMethodId The id of the authentication method the resolved `credentials` are shaped for (see
     * `IAuthenticationConfig::getAuthenticationMethodId`).
     * @param credentials The already-resolved credentials to authenticate with - `createStreaming`
     * has already obtained this (via `requestCredentials`, from a supplied secret or a credential provider)
     * before calling this method, so the implementation only needs to verify it, never to resolve it itself.
     * @returns The created streaming object.
     */
    virtual StreamingPtr onCreateAuthenticatedStreaming(const StringPtr& connectionString,
                                                        const PropertyObjectPtr& config,
                                                        const StringPtr& authenticationMethodId,
                                                        const PropertyObjectPtr& credentials)
    {
        return nullptr;
    }

    virtual Bool onCompleteServerCapability(const ServerCapabilityPtr& source, const ServerCapabilityConfigPtr& target)
    {
        return false;
    }

    virtual ErrCode INTERFACE_FUNC loadLicense(Bool* succeded, IDict* licenseConfig) override
    {
        OPENDAQ_PARAM_NOT_NULL(succeded);
        OPENDAQ_PARAM_NOT_NULL(licenseConfig);

        Bool loadedLicense;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onLoadLicense, loadedLicense, licenseConfig);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        *succeded = loadedLicense;
        return errCode;
    }

    virtual Bool onLoadLicense(IDict* licenseConfig)
    {
        return true;
    }

    virtual ErrCode INTERFACE_FUNC getLicenseConfig(IDict** licenseConfig) override
    {
        OPENDAQ_PARAM_NOT_NULL(licenseConfig);

        DictPtr<IString, IString> licenseConfigLocal;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetLicenseConfig, licenseConfigLocal);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        *licenseConfig = licenseConfigLocal.detach();
        return errCode;
    }

    virtual DictPtr<IString, IString> onGetLicenseConfig()
    {
        return nullptr;
    }

    virtual ErrCode INTERFACE_FUNC licenseLoaded(Bool* valid) override
    {
        OPENDAQ_PARAM_NOT_NULL(valid);

        Bool validLocal;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onLicenseLoaded, validLocal);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        *valid = validLocal;
        return errCode;
    }

    virtual Bool onLicenseLoaded()
    {
        return true;
    }

protected:
    ModuleInfoPtr moduleInfo;

    ContextPtr context;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    Module(StringPtr name, VersionInfoPtr version, ContextPtr context, StringPtr id = nullptr)
        : moduleInfo(ModuleInfo(version, name, id))
        , context(std::move(context))
        , logger(this->context.getLogger())
        , loggerComponent(
              this->logger.assigned()
                  ? this->logger.getOrAddComponent(this->moduleInfo.getName().assigned() ? this->moduleInfo.getName() : "UnknownModule")
                  : throw ArgumentNullException("Logger must not be null"))
    {
    }

    template <typename PopulatePropertiesFunc, typename DiscoveredDeviceT>
    static DeviceInfoPtr populateDiscoveredDeviceInfo(PopulatePropertiesFunc populateProperties,
                                                      const DiscoveredDeviceT& discoveredDevice,
                                                      const ServerCapabilityPtr& cap,
                                                      const DeviceTypePtr& deviceType)
    {
        PropertyObjectPtr deviceInfo = DeviceInfo("");
        populateProperties(deviceInfo, discoveredDevice, ConnectedClientInfo());

        deviceInfo.asPtr<IDeviceInfoInternal>().addServerCapability(cap);
        deviceInfo.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("connectionString", cap.getConnectionString());
        deviceInfo.asPtr<IDeviceInfoConfig>().setDeviceType(deviceType);

        return deviceInfo;
    }

private:
    // Returns `authenticationConfig` unchanged if assigned. Otherwise builds and returns the default config for `componentType`.
    AuthenticationConfigPtr resolveDefaultAuthenticationConfig(const AuthenticationConfigPtr& authenticationConfig, const ComponentTypePtr& componentType)
    {
        if (authenticationConfig.assigned())
            return authenticationConfig;
        return AuthenticationConfig(componentType, context);
    }

    // Builds an `ICredentialRequest` for `authenticationConfig`'s currently selected credential descriptor,
    // then resolves credentials for it via `obtainCredentials` (see it for details on `resolvedProvider`).
    // Throws `AuthenticationFailedException` if `authenticationConfig` is unassigned.
    //
    // A `None`-format method needs no credentials at all, so it is resolved directly here, as unassigned
    // credentials - `createEmptySecret()` doesn't apply to it either, there being no secret to build. No
    // `ICredentialRequest` is even formed for it, since nothing will ever be requested with it: not from a
    // credential provider (none is expected to declare support for `None` - there is nothing for one to
    // provide) and not via a caller-supplied secret either.
    PropertyObjectPtr obtainCredentials(const AuthenticationConfigPtr& authenticationConfig,
                                        const StringPtr& connectionString,
                                        const StringPtr& manufacturer,
                                        const StringPtr& serialNumber,
                                        const ComponentTypePtr& componentType,
                                        CredentialProviderPtr& resolvedProvider)
    {
        if (!authenticationConfig.assigned())
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Authentication is required but no authentication config was provided");

        if (authenticationConfig.getCredentialDescriptor().getFormat() == CredentialFormat::None)
            return nullptr;

        const auto credentialRequest = buildCredentialRequest(authenticationConfig, connectionString, manufacturer, serialNumber, componentType);
        return resolveCredentials(authenticationConfig, credentialRequest, resolvedProvider);
    }

    // Builds an `ICredentialRequest` for `authenticationConfig`'s currently selected credential descriptor,
    // `connectionString` (canonicalized via `onGetCanonicalConnectionString`), `manufacturer`/`serialNumber`,
    // and `componentType`. Split out of `requestCredentials` so `createAuthenticatedDevice` can call it a
    // second time, with manufacturer/serial number resolved from the created device's own info, to re-cache
    // credentials that were originally obtained without them.
    CredentialRequestPtr buildCredentialRequest(const AuthenticationConfigPtr& authenticationConfig,
                                                const StringPtr& connectionString,
                                                const StringPtr& manufacturer,
                                                const StringPtr& serialNumber,
                                                const ComponentTypePtr& componentType)
    {
        auto requestBuilder = CredentialRequestBuilder();
        requestBuilder.setConnectionString(onGetCanonicalConnectionString(connectionString));
        requestBuilder.setManufacturer(manufacturer);
        requestBuilder.setSerialNumber(serialNumber);
        requestBuilder.setDescriptor(authenticationConfig.getCredentialDescriptor());
        requestBuilder.setComponentType(componentType);

        return requestBuilder.build();
    }

    // Resolves the credentials for `credentialRequest` - always a non-`None`-format method (see
    // `requestCredentials`, its only caller, which resolves `None` itself beforehand). Consumes whichever
    // provider id `authenticationConfig.getCredentialProviderId()` currently returns. If
    // `authenticationConfig.getSuppliedSecret()` returns one, it is used directly instead of asking a
    // provider to obtain one - though the currently-selected provider, if any, is still handed the secret
    // via `cacheCredentials`, so a later request for the same context can be served from its cache.
    // `resolvedProvider` receives whichever provider was actually involved, or stays unassigned if none was.
    //
    // Throws `AuthenticationFailedException` if:
    // - `getCredentialProviderId()` returns nothing at all,
    // - the returned provider id no longer names a registered provider,
    // - the returned provider no longer supports the required format, or
    // - the resolved credentials (supplied by the caller, or obtained from a provider) don't match
    //   `credentialDescriptor`'s expected shape.
    PropertyObjectPtr resolveCredentials(const AuthenticationConfigPtr& authenticationConfig,
                                         const CredentialRequestPtr& credentialRequest,
                                         CredentialProviderPtr& resolvedProvider)
    {
        const auto providers = context.getCredentialProviders();
        const auto providerId = authenticationConfig.getCredentialProviderId();
        const PropertyObjectPtr suppliedSecret = authenticationConfig.getSuppliedSecret();

        if (suppliedSecret.assigned())
        {
            if (!secretShapeMatches(suppliedSecret, credentialRequest.getDescriptor()))
                DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Supplied secret does not match the expected shape");

            // Already shaped like the credential descriptor's `createEmptySecret` template (filled in by the
            // caller), so it is used directly as the credential, no provider asked to obtain anything.
            // If a provider is currently selected too, it still gets a chance to cache the secret, so a later
            // interactive request for the same context reuses it.
            if (providerId.assigned())
            {
                resolvedProvider = findMatchingCredentialProvider(providers, credentialRequest.getDescriptor(), providerId);
                if (!resolvedProvider.assigned())
                    DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                         "Authentication is required but no credential provider supporting a compatible format is registered");

                resolvedProvider.cacheCredentials(credentialRequest, suppliedSecret);
            }

            return suppliedSecret;
        }

        resolvedProvider = findMatchingCredentialProvider(providers, credentialRequest.getDescriptor(), providerId);
        if (!resolvedProvider.assigned())
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                 "Authentication is required but no credential provider supporting a compatible format is registered");

        const PropertyObjectPtr credentials = resolvedProvider.requestCredentials(credentialRequest);
        if (!secretShapeMatches(credentials, credentialRequest.getDescriptor()))
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                 "Credential provider \"{}\" returned credentials that do not match the expected shape",
                                 resolvedProvider.getId());

        return credentials;
    }

    // Structural check: does `secret` have exactly the property names `credentialDescriptor.createEmptySecret()`
    // would produce? Doesn't check provenance, only shape.
    static bool secretShapeMatches(const PropertyObjectPtr& secret, const CredentialDescriptorPtr& credentialDescriptor)
    {
        if (!secret.assigned() || !credentialDescriptor.assigned())
            return false;

        const PropertyObjectPtr templateObj = credentialDescriptor.createEmptySecret();
        const auto templateProps = templateObj.getAllProperties();

        if (templateProps.getCount() != secret.getAllProperties().getCount())
            return false;

        for (const auto& prop : templateProps)
        {
            if (!secret.hasProperty(prop.getName()))
                return false;
        }

        return true;
    }

    static bool supportsCredentialFormat(const CredentialProviderPtr& provider, const CredentialDescriptorPtr& credentialDescriptor)
    {
        for (const auto& format : provider.getSupportedFormats())
        {
            if (static_cast<CredentialFormat>(static_cast<Int>(format)) == credentialDescriptor.getFormat())
                return true;
        }

        return false;
    }

    // Looks up `providerId` among the registered `providers` and validates it supports `credentialDescriptor`'s
    // format. `providerId` is expected to already be whatever `authenticationConfig.getCredentialProviderId()`
    // currently returns, pre-filtered to compatible providers. An unassigned `providerId` simply returns
    // unassigned; it is the caller's job (`obtainCredentials`) to treat that as a failure.
    //
    // Throws `AuthenticationFailedException` if `providerId` is assigned but:
    // - names no registered provider, or
    // - names a provider that no longer supports the required format.
    static CredentialProviderPtr findMatchingCredentialProvider(const DictPtr<IString, ICredentialProvider>& providers,
                                                                const CredentialDescriptorPtr& credentialDescriptor,
                                                                const StringPtr& providerId = nullptr)
    {
        if (!providerId.assigned())
            return nullptr;

        if (!providers.assigned() || !providers.hasKey(providerId))
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                 "Authentication is required but the selected credential provider \"{}\" is not registered",
                                 providerId);
        }

        auto provider = providers.get(providerId);
        if (!supportsCredentialFormat(provider, credentialDescriptor))
        {
            DAQ_THROW_EXCEPTION(
                AuthenticationFailedException,
                "Authentication is required but the selected credential provider \"{}\" does not support the required format",
                providerId);
        }

        return provider;
    }

    StringPtr getPrefixFromConnectionString(const StringPtr& connectionString) const
    {
        try
        {
            std::string str = connectionString;
            return str.substr(0, str.find("://"));
        }
        catch(...)
        {
            LOG_W("Connection string has no prefix denoted by the \"://\" delimiter")
        }

        return "";
    }

    static void populateDefaultConfig(const PropertyObjectPtr& defaultObj, const PropertyObjectPtr& userInput)
    {
        for (const auto& prop : defaultObj.getAllProperties())
            {
                const auto propName = prop.getName();

                if (userInput.hasProperty(propName))
                {
                    const auto userProp = userInput.getProperty(propName);
                    const auto defaultProp = defaultObj.getProperty(propName);

                    if (userProp.getValueType() != defaultProp.getValueType())
                        continue;

                    if (userProp.getValueType() == ctObject)
                        populateDefaultConfig(defaultProp.getValue(), userProp.getValue());
                    else
                        defaultObj.setPropertyValue(propName, userProp.getValue());
                }
            }
    }

    PropertyObjectPtr mergeConfig(const PropertyObjectPtr& userConfig, const ComponentTypePtr& type) const
    {
        PropertyObjectPtr configIn = userConfig.assigned() ? userConfig : PropertyObject();
        PropertyObjectPtr configOut;

        try
        {
            auto errorGuard = DAQ_ERROR_GUARD();
            configOut = type.assigned() ? type.createDefaultConfig() : PropertyObject();
            populateDefaultConfig(configOut, configIn);
        }
        catch (const DaqException& e)
        {
            LOG_W("Failed to merge configuration: {}", e.what())
            return configIn;
        }
        catch (const std::exception& e)
        {
            LOG_W("Failed to merge configuration: {}", e.what())
            return configIn;
        }

        return configOut;
    }
};

END_NAMESPACE_OPENDAQ
