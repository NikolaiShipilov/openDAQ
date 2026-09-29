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
#include <opendaq/authentication_method.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>", true)]
 */

/*!
 * @brief Carries the authentication settings used for a single connection attempt to a component.
 *
 * A component created with authentication keeps the config it was authenticated with (see
 * `IComponentPrivate::setAuthenticationConfig`), so that reloading it later goes through the same
 * credential-request process again.
 *
 * Is itself a Property object - the authentication method id and the `IAuthenticationMethod` it corresponds to
 * are bound together as one `"AuthenticationMethod"` Selection property (its selection value is the
 * `IAuthenticationMethod` Struct itself, so the two can never be set out of sync - the authentication
 * method id is simply the selected authentication method's own `IAuthenticationMethod::getId()`).
 * A directly-supplied secret is carried, when present, as a `"SuppliedSecret"` property, validated on every
 * write against whatever `"AuthenticationMethod"` is currently selected (its property names must match
 * the selected authentication method's `createEmptySecret()`'s exactly - the blessed workflow is to build from that template, fill
 * it in, and submit it) - a mismatched write is rejected, and an already-set `"SuppliedSecret"` that a
 * `"AuthenticationMethod"` change leaves incompatible is silently cleared. The typed getters/setters below
 * are an equal, typed alternative to tuning the config through these properties directly - a caller can
 * customize it either way, entirely through `IAuthenticationConfig` itself or entirely through the generic
 * `IPropertyObject` interface this object also implements; `"SuppliedSecret"` and `"AuthenticationMethod"`
 * can both equally be read and set through either.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfig, IPropertyObject)
{
    /*!
     * @brief Gets the id of the authentication method currently selected - the selected
     * `"AuthenticationMethod"` property value's own `IAuthenticationMethod::getId()`.
     * @param[out] authenticationMethodId The authentication method id.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedAuthenticationMethodId(IString** authenticationMethodId) = 0;

    /*!
     * @brief Selects the authentication method to use, by its own id - the typed equivalent of
     * `setPropertySelectionValue("AuthenticationMethod", authenticationMethod)`: looks the matching
     * authentication method up among `getSupportedAuthenticationMethods()` internally, so the caller only ever needs
     * to name the id, never the Struct itself. Selecting a new method may clear an incompatible
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
     * @param[out] authenticationMethods The supported authentication methods, keyed by their own id.
     */
    // [templateType(authenticationMethods, IString, IAuthenticationMethod)]
    virtual ErrCode INTERFACE_FUNC getSupportedAuthenticationMethods(IDict** authenticationMethods) = 0;

    /*!
     * @brief Gets the secret supplied directly by the caller - the value of the corresponding property.
     * @param[out] secret The supplied secret, or `nullptr` if the config has no such property
     * at all - in which case the module obtains one from the registered credential provider instead.
     */
    virtual ErrCode INTERFACE_FUNC getSuppliedSecret(IPropertyObject** secret) = 0;
};

/*!
 * @brief Builds an `AuthenticationConfig` supporting every authentication method described in
 * `authenticationMethods`. Each entry becomes one candidate value of the resulting config's
 * `"AuthenticationMethod"` Selection property (see `IAuthenticationConfig`); its first entry,
 * in dict iteration order, starts out selected.
 *
 * A config that only ever supports one method is simply the one-entry case of this: pass a
 * `authenticationMethods` dict with a single key/value pair.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, authenticationMethods
)

END_NAMESPACE_OPENDAQ
