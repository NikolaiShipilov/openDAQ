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
#include <opendaq/authentication_config.h>
#include <coreobjects/property_object_impl.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/credential_payload_descriptor_ptr.h>
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AuthenticationConfigImpl : public GenericPropertyObjectImpl<IAuthenticationConfig>
{
public:
    using Super = GenericPropertyObjectImpl<IAuthenticationConfig>;

    // `context` and `typeId` have no default - every caller states explicitly whether it has one (`nullptr`
    // included), rather than a config silently ending up context-less or type-less by omission. `typeId` is
    // what lets a saved config re-resolve `payloadDescriptors`/`context` fresh on reload (see
    // `serialize`/`Deserialize`) - a config built with no type behind it can't meaningfully round-trip
    // through save/reload. This is the class's one real constructor (no overloads compete for its
    // arguments), so plain smart pointers are used, matching impl ctors elsewhere in openDAQ.
    AuthenticationConfigImpl(const DictPtr<IString, ICredentialPayloadDescriptor>& payloadDescriptors,
                             const StringPtr& defaultPayloadId,
                             const ContextPtr& context,
                             const StringPtr& typeId);

    ErrCode INTERFACE_FUNC getCredentialPayloadId(IString** payloadId) override;
    ErrCode INTERFACE_FUNC getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getCredentialProviderId(IString** providerId) override;
    ErrCode INTERFACE_FUNC getSuppliedSecret(IPropertyObject** secret) override;

    // Intercepted to keep "CredentialProviderId" a live slave of "PayloadDescriptor" (recomputed from
    // `context` on every master write, added/removed as compatibility changes), to remember the user's last
    // explicit provider choice, and to validate/auto-clear "SuppliedSecret" against whichever descriptor is
    // currently selected.
    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setPropertySelectionValue(IString* propertyName, IBaseObject* value) override;

    // Fully custom - replaces generic PropertyObject serialization entirely. Only `typeId` and the selected
    // payload id are written; "CredentialProviderId" (not portable across runs) and "SuppliedSecret" (a
    // secret) are never serialized, regardless of whether they're currently present.
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    static constexpr const char* PayloadDescriptorPropertyName = "PayloadDescriptor";
    static constexpr const char* CredentialProviderIdPropertyName = "CredentialProviderId";
    static constexpr const char* SuppliedSecretPropertyName = "SuppliedSecret";
    static constexpr const char* TypeIdSerializedKey = "TypeId";
    static constexpr const char* PayloadIdSerializedKey = "PayloadId";

    ContextPtr context;
    StringPtr typeId;
    StringPtr preferredCredentialProviderId;

    void initProperties(const DictPtr<IString, ICredentialPayloadDescriptor>& payloadDescriptors,
                        const StringPtr& defaultPayloadId);

    // No-op when `context` is unassigned - there's no live provider list to filter without it, so
    // "CredentialProviderId" is simply never present on such a config. Otherwise always re-queries
    // `context.getCredentialProviders()` fresh (never a cached snapshot), filters by `selectedDescriptor`'s
    // format, and adds/removes/replaces "CredentialProviderId" to match.
    void rebuildCredentialProviderCandidates(const CredentialPayloadDescriptorPtr& selectedDescriptor);

    void clearSuppliedSecretIfIncompatible(const CredentialPayloadDescriptorPtr& selectedDescriptor);

    // Structural check: does `secret` have exactly the property names `selectedDescriptor.createDefaultPayload()`
    // would produce? The blessed workflow is to build from that template and fill it in, but this doesn't
    // check provenance, only shape.
    static bool IsSuppliedSecretShapeValid(const PropertyObjectPtr& secret, const CredentialPayloadDescriptorPtr& selectedDescriptor);
};

END_NAMESPACE_OPENDAQ
