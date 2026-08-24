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
#include <opendaq/credential_request.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 */

/*!
 * @brief Carries the authentication settings used for a single connection attempt to a component.
 *
 * Credential settings do not live in the base add-component config or its default - they travel in a
 * dedicated authentication config object that exists alongside base config and is never serialized.
 *
 * An authentication config carries:
 * - The selected payload - its id and descriptor.
 * - The additional config - a property object, defined by the component type alongside the payload, carrying
 *   settings that might travel with the credential request to the provider (e.g. whether to hide secret input as it
 *   is typed) and, in some cases, even directly supplied credentials (e.g. a certificate file path).
 *   It is supplied for this connection attempt only, and is never saved.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfig, IBaseObject)
{
    /*!
     * @brief Gets the id of the payload associated with selected authentication method.
     * @param[out] payloadId The payload id.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialPayloadId(IString** payloadId) = 0;

    /*!
     * @brief Gets the descriptor of the payload which selected authentication method uses.
     * @param[out] descriptor The payload descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) = 0;

    /*!
     * @brief Gets additional configuration specific to selected authentication method.
     * @param[out] config The configuration property object.
     */
    virtual ErrCode INTERFACE_FUNC getConfig(IPropertyObject** config) = 0;

    /*!
     * @brief Gets the id of the credential provider to request credentials from.
     * @param[out] providerId The credential provider id, or `nullptr` if none was explicitly selected - in
     * which case the module auto-selects a registered provider supporting the payload descriptor's format,
     * same as when this is left unset.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialProviderId(IString** providerId) = 0;

    /*!
     * @brief Gets the authentication configs nested under this one, accumulated via
     * `IAuthenticationConfigBuilder::addStreamingAuthenticationConfig` - so a single authentication config,
     * formed for connecting to a device, can also carry the settings needed to authenticate a streaming
     * source attached to that device. Each is an ordinary authentication config in its own right, keyed by
     * the streaming type's own id.
     * @param[out] streamingAuthenticationConfigs The streaming type id -> authentication config dictionary.
     */
    // [templateType(streamingAuthenticationConfigs, IString, IAuthenticationConfig)]
    virtual ErrCode INTERFACE_FUNC getStreamingAuthenticationConfigs(IDict** streamingAuthenticationConfigs) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IString*, payloadId, ICredentialPayloadDescriptor*, payloadDescriptor, IPropertyObject*, config
)

/*!
 * @brief Reconstructs an `AuthenticationConfig` from a previously formed, saved `CredentialRequest` - used
 * only when reloading a saved device that had previously been added with authentication. Never exposed to
 * other language bindings; not meant for regular user code, which should use the `AuthenticationConfig`
 * factory above instead.
 */
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfigFromCredentialRequest, IAuthenticationConfig,
    ICredentialRequest*, credentialRequest
)

END_NAMESPACE_OPENDAQ
