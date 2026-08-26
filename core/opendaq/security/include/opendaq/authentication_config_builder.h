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
#include <coretypes/baseobject.h>
#include <coreobjects/property_object.h>
#include <opendaq/credential_payload_descriptor.h>
#include <opendaq/authentication_config.h>
#include <opendaq/streaming_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 * [interfaceLibrary(IStreamingType, "opendaq")]
 * [interfaceSmartPtr(IStreamingType, StreamingTypePtr, "<opendaq/streaming_type_ptr.h>")]
 */

/*!
 * @brief Builds `IAuthenticationConfig` objects.
 *
 * Besides the payload id/descriptor/config that make up a plain authentication config, a builder can also
 * accumulate authentication configs nested under a streaming type - so a single authentication config,
 * formed for connecting to a device, can also carry the settings needed to authenticate a streaming source
 * attached to that device. Each nested config, once retrieved via
 * `IAuthenticationConfig::getStreamingAuthenticationConfigs`, is an ordinary authentication config in its
 * own right, usable anywhere a standalone one would be (e.g. passed directly to a manual `addStreaming`
 * call).
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfigBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns an `AuthenticationConfig` using the currently configured values.
     * @param[out] authenticationConfig The built authentication config.
     */
    virtual ErrCode INTERFACE_FUNC build(IAuthenticationConfig** authenticationConfig) = 0;

    /*!
     * @brief Sets the id of the payload associated with the selected authentication method.
     * @param payloadId The payload id.
     */
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setPayloadId(IString* payloadId) = 0;

    /*!
     * @brief Gets the id of the payload associated with the selected authentication method.
     * @param[out] payloadId The payload id.
     */
    virtual ErrCode INTERFACE_FUNC getPayloadId(IString** payloadId) = 0;

    /*!
     * @brief Sets the descriptor of the payload the selected authentication method uses.
     * @param descriptor The payload descriptor.
     */
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setPayloadDescriptor(ICredentialPayloadDescriptor* descriptor) = 0;

    /*!
     * @brief Gets the descriptor of the payload the selected authentication method uses.
     * @param[out] descriptor The payload descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) = 0;

    /*!
     * @brief Sets additional configuration specific to the selected authentication method.
     * @param config The configuration property object.
     */
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setConfig(IPropertyObject* config) = 0;

    /*!
     * @brief Gets additional configuration specific to the selected authentication method.
     * @param[out] config The configuration property object.
     */
    virtual ErrCode INTERFACE_FUNC getConfig(IPropertyObject** config) = 0;

    /*!
     * @brief Sets the id of the credential provider to request credentials from.
     * @param providerId The credential provider id, or `nullptr` to let the module auto-select a
     * registered provider supporting the payload descriptor's format - the default when left unset.
     */
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setCredentialProviderId(IString* providerId) = 0;

    /*!
     * @brief Gets the id of the credential provider to request credentials from.
     * @param[out] providerId The credential provider id.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialProviderId(IString** providerId) = 0;

    /*!
     * @brief Sets a secret to use directly instead of a credential provider obtaining it (e.g. by
     * prompting the user).
     * @param suppliedSecret The secret - a property object built from `setPayloadDescriptor`'s
     * `createDefaultPayload` template and filled in with the actual secret value(s). `nullptr` (the
     * default) leaves the module to obtain the secret from a credential provider as usual.
     */
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setSuppliedSecret(IPropertyObject* suppliedSecret) = 0;

    /*!
     * @brief Gets the secret to use directly instead of a credential provider obtaining it.
     * @param[out] suppliedSecret The supplied secret.
     */
    virtual ErrCode INTERFACE_FUNC getSuppliedSecret(IPropertyObject** suppliedSecret) = 0;

    /*!
     * @brief Adds (or replaces) the authentication config nested under the given streaming type - keyed
     * internally by the type's own id (`IComponentType::getId`), so only an id a real, registered streaming
     * type actually has can ever be used as the key, rather than an arbitrary caller-supplied string.
     * @param streamingType The streaming type the nested config is for.
     * @param streamingAuthenticationConfig The nested authentication config.
     */
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC addStreamingAuthenticationConfig(IStreamingType* streamingType, IAuthenticationConfig* streamingAuthenticationConfig) = 0;

    /*!
     * @brief Gets the nested authentication configs accumulated via `addStreamingAuthenticationConfig`.
     * @param[out] streamingAuthenticationConfigs The streaming type id -> authentication config dictionary.
     */
    // [templateType(streamingAuthenticationConfigs, IString, IAuthenticationConfig)]
    virtual ErrCode INTERFACE_FUNC getStreamingAuthenticationConfigs(IDict** streamingAuthenticationConfigs) = 0;
};

/*!
 * @brief Creates an `AuthenticationConfigBuilder` with no values set.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfigBuilder, IAuthenticationConfigBuilder
)

END_NAMESPACE_OPENDAQ
