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
     * @brief Gets the id of the credential provider to request credentials from.
     * @param[out] providerId The credential provider id, or `nullptr` if none was explicitly selected - in
     * which case the module auto-selects a registered provider supporting the payload descriptor's format,
     * same as when this is left unset.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialProviderId(IString** providerId) = 0;

    /*!
     * @brief Gets the secret supplied directly by the caller, to be used instead of a credential provider
     * obtaining it (e.g. by prompting the user).
     * @param[out] suppliedSecret The supplied secret - a property object shaped like
     * `getCredentialPayloadDescriptor()`'s `createDefaultPayload` template, filled in with the actual
     * secret value(s) - or `nullptr` (the default) if none was supplied, in which case the module obtains
     * the secret from a credential provider as usual.
     */
    virtual ErrCode INTERFACE_FUNC getSuppliedSecret(IPropertyObject** suppliedSecret) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IString*, payloadId, ICredentialPayloadDescriptor*, payloadDescriptor
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
