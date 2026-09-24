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
#include <opendaq/credential_descriptor.h>
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
 * Is itself a Property object - the authentication method id and its corresponding credential descriptor are bound
 * together as one `"AuthenticationMethod"` Selection property (its selection value is the
 * `ICredentialDescriptor` Struct itself, so the two can never be set out of sync - the authentication
 * method id is simply the selected descriptor's own `ICredentialDescriptor::getAuthenticationMethodId()`). Its current selection
 * drives `"CredentialProviderId"`'s own candidates: `"CredentialProviderId"` is a Selection over
 * `Context::getCredentialProviders()` filtered live to the *currently selected* `"AuthenticationMethod"`'s
 * format - re-queried from `Context` and recomputed (candidates added, removed, or refreshed) every time
 * `"AuthenticationMethod"` is written, never a cached snapshot. The property is entirely absent whenever no
 * registered provider currently supports the selected format.
 * A directly-supplied secret is carried, when present, as a `"SuppliedSecret"` property, validated on every
 * write against whatever `"AuthenticationMethod"` is currently selected (its property names must match
 * `descriptor.createEmptySecret()`'s exactly - the blessed workflow is to build from that template, fill
 * it in, and submit it) - a mismatched write is rejected, and an already-set `"SuppliedSecret"` that a
 * `"AuthenticationMethod"` change leaves incompatible is silently cleared. The typed getters/setters below
 * are an equal, typed alternative to tuning the config through these properties directly - a caller can
 * customize it either way, entirely through `IAuthenticationConfig` itself or entirely through the generic
 * `IPropertyObject` interface this object also implements; `"SuppliedSecret"`, `"CredentialProviderId"`, and
 * `"AuthenticationMethod"` can all equally be read and set through either.
 *
 * Serialization relies on the generic `IPropertyObject` mechanism for `"AuthenticationMethod"` only - every
 * candidate credential descriptor and the selected one round-trip through it like any other property.
 * `"CredentialProviderId"` and `"SuppliedSecret"` are both excluded from it: `"SuppliedSecret"` (a secret) is
 * never persisted at all; `"CredentialProviderId"`'s *candidates* are never persisted either, since they're
 * always live-recomputed from the current `Context` and a saved snapshot could be stale - only its *selected*
 * value (if any) is written, as one extra value that isn't itself a property. Deserializing can't use the
 * generic property-object reconstruction pipeline as-is (this class has no default constructor), so it reads
 * the saved credential descriptors/method id directly off `"AuthenticationMethod"`'s own serialized definition
 * first, to resolve a `Context` and rebuild the config through the same constructor a fresh one goes through -
 * the saved `"CredentialProviderId"` selection, if any, is then reapplied only if it's still among the
 * freshly, live-recomputed candidates.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfig, IPropertyObject)
{
    /*!
     * @brief Gets the id of the authentication method currently selected - the selected
     * `"AuthenticationMethod"` property value's own `ICredentialDescriptor::getAuthenticationMethodId()`.
     * @param[out] authenticationMethodId The authentication method id.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedAuthenticationMethodId(IString** authenticationMethodId) = 0;

    /*!
     * @brief Selects the authentication method to use, by its own id - the typed equivalent of
     * `setPropertySelectionValue("AuthenticationMethod", descriptor)`: looks the matching credential
     * descriptor up among `getSupportedAuthenticationMethods()` internally, so the caller only ever needs
     * to name the id, never the Struct itself. Selecting a new method live-recomputes
     * `"CredentialProviderId"`'s own candidates (see `IAuthenticationConfig`) and may clear an incompatible
     * `"SuppliedSecret"`.
     * @param authenticationMethodId The id of one of `getSupportedAuthenticationMethods()`'s own keys.
     * @throws NotFoundException if `authenticationMethodId` doesn't match any of this config's supported
     * authentication methods.
     */
    virtual ErrCode INTERFACE_FUNC setAuthenticationMethodId(IString* authenticationMethodId) = 0;

    /*!
     * @brief Gets every authentication method this config supports, keyed by their own id - the full set of
     * `"AuthenticationMethod"` selection candidates, i.e. the same shape `IComponentType::getSupportedAuthenticationMethods()`
     * has, since this config was built from exactly that set. An equal, typed alternative to reading the
     * candidates generically off the `"AuthenticationMethod"` property.
     * @param[out] descriptors The supported authentication credential descriptors, keyed by their own id.
     */
    // [templateType(descriptors, IString, ICredentialDescriptor)]
    virtual ErrCode INTERFACE_FUNC getSupportedAuthenticationMethods(IDict** descriptors) = 0;

    /*!
     * @brief Gets the id of the credential provider to request credentials from - the current selection
     * value of the `"CredentialProviderId"` property (see `IAuthenticationConfig`).
     * @param[out] providerId The credential provider id, or `nullptr` if the config has no
     * `"CredentialProviderId"` property at all - which only happens when no registered provider supports the
     * currently selected descriptor's format; so authentication simply fails.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedCredentialProviderId(IString** providerId) = 0;

    /*!
     * @brief Selects the credential provider to request credentials from, by its own id - the typed
     * equivalent of `setPropertySelectionValue("CredentialProviderId", providerId)`.
     * @param providerId The id of one of `getSupportedCredentialProviderIds()`'s own entries.
     * @throws NotFoundException if the config currently has no `"CredentialProviderId"` property at all (no
     * registered provider currently supports the selected method's format).
     * @throws InvalidParameterException if `providerId` doesn't match any of the current candidates.
     */
    virtual ErrCode INTERFACE_FUNC setCredentialProviderId(IString* providerId) = 0;

    /*!
     * @brief Gets the ids of every credential provider currently compatible with the selected authentication
     * method's format - the full set of `"CredentialProviderId"` selection candidates, live-recomputed from
     * `Context` every time the selected `"AuthenticationMethod"` changes (see `IAuthenticationConfig`). An
     * equal, typed alternative to reading the candidates generically off the `"CredentialProviderId"`
     * property.
     * @param[out] providerIds The compatible credential provider ids - empty if the config currently has no
     * `"CredentialProviderId"` property at all (no registered provider currently supports the selected
     * method's format).
     */
    // [templateType(providerIds, IString)]
    virtual ErrCode INTERFACE_FUNC getSupportedCredentialProviderIds(IList** providerIds) = 0;

    /*!
     * @brief Gets the secret supplied directly by the caller - the value of the corresponding property.
     * @param[out] secret The supplied secret, or `nullptr` if the config has no such property
     * at all - in which case the module obtains one from a credential provider instead.
     */
    virtual ErrCode INTERFACE_FUNC getSuppliedSecret(IPropertyObject** secret) = 0;
};

/*!
 * @brief Builds an `AuthenticationConfig` supporting every authentication method described in
 * `credentialDescriptors`. Each entry becomes one candidate value of the resulting config's
 * `"AuthenticationMethod"` Selection property (see `IAuthenticationConfig`), so a caller can later switch
 * between methods just by changing that property's selection, rather than needing a different config
 * object per method. `credentialDescriptors` is a dict keyed by each descriptor's own
 * `ICredentialDescriptor::getAuthenticationMethodId()`; its first entry, in dict iteration order, starts
 * out selected.
 *
 * A config that only ever supports one method is simply the one-entry case of this: pass a
 * `credentialDescriptors` dict with a single key/value pair.
 *
 * This is the lower-level overload, for building a config with no live `IComponentType` object at hand
 * (e.g. deserialization). To build one for an actual device/streaming type, prefer the
 * `AuthenticationConfig(componentType, context)` overload (see `authentication_config_factory.h`), which
 * reads `credentialDescriptors` straight off the type itself.
 * @param context The `Context` to live-filter `"CredentialProviderId"`'s candidates from (see
 * `IAuthenticationConfig`) - must be assigned. Throws otherwise.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, credentialDescriptors, IContext*, context
)

END_NAMESPACE_OPENDAQ
