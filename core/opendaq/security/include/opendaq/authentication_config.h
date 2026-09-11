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
#include <opendaq/context.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>", true)]
 * [interfaceLibrary(IContext, "opendaq")]
 */

/*!
 * @brief Carries the authentication settings used for a single connection attempt to a component.
 *
 * A component created with authentication keeps the config it was authenticated with (see
 * `IComponentPrivate::setAuthenticationConfig`), so that reloading it later goes through the same
 * credential-request process again.
 *
 * Is itself a Property object - the payload id and its descriptor are bound
 * together as one `"PayloadDescriptor"` Selection property (its selection value is the
 * `ICredentialPayloadDescriptor` Struct itself, so the two can never be set out of sync - the payload id
 * is simply the selected descriptor's own `ICredentialPayloadDescriptor::getId()`). Its current selection
 * drives `"CredentialProviderId"`'s own candidates: `"CredentialProviderId"` is a Selection over
 * `Context::getCredentialProviders()` filtered live to the *currently selected* `"PayloadDescriptor"`'s
 * format - re-queried from `Context` and recomputed (candidates added, removed, or refreshed) every time
 * `"PayloadDescriptor"` is written, never a cached snapshot. The property is entirely absent whenever no
 * registered provider currently supports the selected format.
 * A directly-supplied secret is carried, when present, as a `"SuppliedSecret"` property, validated on every
 * write against whatever `"PayloadDescriptor"` is currently selected (its property names must match
 * `descriptor.createDefaultPayload()`'s exactly - the blessed workflow is to build from that template, fill
 * it in, and submit it) - a mismatched write is rejected, and an already-set `"SuppliedSecret"` that a
 * `"PayloadDescriptor"` change leaves incompatible is silently cleared. The typed getters below are a
 * convenience layer on top of these properties; `"SuppliedSecret"`, `"CredentialProviderId"`, and
 * `"PayloadDescriptor"` can all equally be read and set through the ordinary `IPropertyObject` interface this
 * object also implements.
 *
 * Serialization is fully custom, not the generic `IPropertyObject` mechanism: the component type id (as
 * passed by `IDevice::createDefaultAuthenticationConfig`), the selected `"PayloadDescriptor"`'s id, and -
 * when present - the selected `"CredentialProviderId"` are written. `"SuppliedSecret"` (a secret) is never
 * serialized. Deserializing re-resolves the saved type id against the live `Context` and rebuilds everything
 * above fresh - it fails outright if the type no longer resolves, or the saved payload id is no longer
 * among that type's currently supported descriptors. The saved provider id is restored only if it's still
 * among the freshly-rebuilt `"CredentialProviderId"` candidates; otherwise the normal live default applies.
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
     * @brief Gets the id of the credential provider to request credentials from - the current selection
     * value of the `"CredentialProviderId"` property (see `IAuthenticationConfig`).
     * @param[out] providerId The credential provider id, or `nullptr` if the config has no
     * `"CredentialProviderId"` property at all - which only happens when no registered provider supports the
     * currently selected payload's format; so authentication simply fails.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialProviderId(IString** providerId) = 0;

    /*!
     * @brief Gets the secret supplied directly by the caller - the value of the corresponding property.
     * @param[out] secret The supplied secret, or `nullptr` if the config has no such property
     * at all - in which case the module obtains one from a credential provider instead.
     */
    virtual ErrCode INTERFACE_FUNC getSuppliedSecret(IPropertyObject** secret) = 0;
};

/*!
 * @brief Builds an `AuthenticationConfig` supporting every authentication method described in
 * `payloadDescriptors`. Each entry becomes one candidate value of the resulting config's
 * `"PayloadDescriptor"` Selection property (see `IAuthenticationConfig`), so a caller can later switch
 * between methods just by changing that property's selection, rather than needing a different config
 * object per method. `payloadDescriptors` is a dict keyed by each descriptor's own
 * `ICredentialPayloadDescriptor::getId()`; `defaultPayloadId` names which one of those keys starts out
 * selected.
 *
 * A config that only ever supports one method is simply the one-entry case of this: pass a
 * `payloadDescriptors` dict with a single key/value pair.
 * @param context The `Context` to live-filter `"CredentialProviderId"`'s candidates from (see
 * `IAuthenticationConfig`) - must be assigned. Throws otherwise.
 * @param typeId The id of the component type this config was built for - carried through serialization so a
 * reload can re-resolve `payloadDescriptors`/`context` fresh (see `IAuthenticationConfig`'s serialization
 * notes) - required, no default; pass `nullptr` explicitly for a config with no type behind it (e.g. one
 * built for its own sake, not via `IDevice::createDefaultAuthenticationConfig`), which then cannot
 * meaningfully round-trip through save/reload.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, payloadDescriptors, IString*, defaultPayloadId, IContext*, context, IString*, typeId
)

END_NAMESPACE_OPENDAQ
