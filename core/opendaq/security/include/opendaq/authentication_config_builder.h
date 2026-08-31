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

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 */

/*!
 * @brief Builds `IAuthenticationConfig` objects.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfigBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns an `AuthenticationConfig` using the currently configured values.
     * @param[out] authenticationConfig The built authentication config.
     */
    virtual ErrCode INTERFACE_FUNC build(IAuthenticationConfig** authenticationConfig) = 0;

    /*!
     * @brief Sets the descriptor of the payload the selected authentication method uses - its own
     * `ICredentialPayloadDescriptor::getId()` becomes the built config's payload id.
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
     * @brief Sets the id of the credential provider to request credentials from.
     * @param providerId The credential provider id, or `nullptr` to let the module auto-select a
     * registered provider supporting the payload descriptor's format - the default when left unset.
     *
     * The builder has no `Context` access to enumerate registered providers, so the built config's
     * `"CredentialProviderId"` property (see `IAuthenticationConfig`) is a plain String here rather than a
     * Selection - there's no candidate list to select from, only this one explicit value (or none at all).
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
};

/*!
 * @brief Creates an `AuthenticationConfigBuilder` with no values set.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfigBuilder, IAuthenticationConfigBuilder
)

END_NAMESPACE_OPENDAQ
