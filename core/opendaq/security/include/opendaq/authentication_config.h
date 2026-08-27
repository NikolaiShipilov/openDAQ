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

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

/*!
 * @brief Carries the authentication settings used for a single connection attempt to a component.
 *
 * Credential settings do not live in the base add-component config or its default - they travel in a
 * dedicated authentication config object that exists alongside base config. A component created with
 * authentication may persist the whole config it was authenticated with alongside itself (see
 * `IComponentPrivate::setAuthenticationConfig`), so a reload can re-request credentials for it.
 *
 * Is itself a Property object (like `IDeviceInfo`) - the payload id and its descriptor are bound
 * together as one `"PayloadDescriptor"` Selection property (its selection value is the
 * `ICredentialPayloadDescriptor` Struct itself, so the two can never be set out of sync - the payload id
 * is simply the selected descriptor's own `ICredentialPayloadDescriptor::getId()`), the credential
 * provider id is a plain `"CredentialProviderId"` String property, and a directly-supplied secret (see
 * `IAuthenticationConfigBuilder::setSuppliedSecret`) is carried, when present, as a `"SuppliedSecret"`
 * property (absent when none was supplied). The typed getters below are a convenience layer on top of the
 * first two properties; `"SuppliedSecret"`, `"CredentialProviderId"`, and `"PayloadDescriptor"` can all
 * equally be read (and, via `IAuthenticationConfigBuilder`-built instances, set) through the ordinary
 * `IPropertyObject` interface this object also implements.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfig, IPropertyObject)
{
    /*!
     * @brief Gets the id of the payload associated with selected authentication method - the selected
     * `"PayloadDescriptor"` property value's own `ICredentialPayloadDescriptor::getId()`.
     * @param[out] payloadId The payload id.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialPayloadId(IString** payloadId) = 0;

    /*!
     * @brief Gets the descriptor of the payload which selected authentication method uses - the current
     * selection value of the `"PayloadDescriptor"` property.
     * @param[out] descriptor The payload descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) = 0;

    /*!
     * @brief Gets the id of the credential provider to request credentials from - the value of the
     * `"CredentialProviderId"` String property.
     * @param[out] providerId The credential provider id, or `nullptr` if none was explicitly selected - in
     * which case the module auto-selects a registered provider supporting the payload descriptor's format,
     * same as when this is left unset.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialProviderId(IString** providerId) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    ICredentialPayloadDescriptor*, payloadDescriptor
)

/*!
 * @brief Builds a self-contained `AuthenticationConfig` listing every one of `payloadDescriptors` as a
 * candidate of its `"PayloadDescriptor"` selection property, defaulting to the one whose own
 * `ICredentialPayloadDescriptor::getId()` matches `defaultPayloadId` - used by
 * `IComponentType::createDefaultAuthenticationConfig` to hand the caller one config object covering every
 * authentication method the component type supports, rather than a separate config per method. Never
 * exposed to other language bindings; not meant for regular user code.
 */
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfigFromSupportedMethods, IAuthenticationConfig,
    IList*, payloadDescriptors, IString*, defaultPayloadId
)

END_NAMESPACE_OPENDAQ
