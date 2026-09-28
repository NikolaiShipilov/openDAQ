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
#include <opendaq/credential_descriptor_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AuthenticationConfigImpl : public GenericPropertyObjectImpl<IAuthenticationConfig>
{
public:
    using Super = GenericPropertyObjectImpl<IAuthenticationConfig>;

    // The first entry of `credentialDescriptors` (in dict iteration order) starts out selected.
    explicit AuthenticationConfigImpl(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors);

    // Stub constructor used only by `Deserialize` (mirrors `DeviceInfoConfigImpl`'s own default constructor) -
    // produces an object with no properties at all yet, not even "AuthenticationMethod", so the generic
    // PropertyObject deserialization pipeline can add it back fresh from its own serialized definition, the
    // same way it would for any other Property. None of the typed getters/setters are valid on an object built
    // this way until that happens - not meant for direct use otherwise.
    AuthenticationConfigImpl();

    ErrCode INTERFACE_FUNC getSelectedAuthenticationMethodId(IString** authenticationMethodId) override;
    ErrCode INTERFACE_FUNC setAuthenticationMethodId(IString* authenticationMethodId) override;
    ErrCode INTERFACE_FUNC getSupportedAuthenticationMethods(IDict** descriptors) override;
    ErrCode INTERFACE_FUNC getSuppliedSecret(IPropertyObject** secret) override;

    // Intercepted to validate/auto-clear "SuppliedSecret" against whichever descriptor is currently selected
    // whenever "AuthenticationMethod" changes. `setProtectedPropertyValue` gets the same treatment (see
    // `onPropertyValueChanged`) since it's the path generic deserialization (`DeserializePropertyValues`) writes
    // through, bypassing `setPropertyValue`/`setPropertySelectionValue` entirely.
    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setPropertySelectionValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;

    // Relies on the generic PropertyObject serialization for "AuthenticationMethod" (its candidates and
    // selected value round-trip through it correctly on their own - see `serializeProperty`/
    // `serializePropertyValue`) but not for "SuppliedSecret" - a secret, so it must never be persisted at all
    // (same two overrides, excluding it).
    // Deserializing builds a bare stub first (the constructor above), then runs the entirely generic
    // PropertyObject pipeline on it: `DeserializePropertyOrder`/`DeserializeLocalProperties`/
    // `DeserializePropertyValues` add "AuthenticationMethod" back from its own serialized definition (candidates
    // and all) and restore its saved selection override, if one was saved - routed through
    // `setProtectedPropertyValue` (see above). No manual JSON parsing of its own.
    ErrCode serializeProperty(const PropertyPtr& property, ISerializer* serializer) override;
    ErrCode serializePropertyValue(const StringPtr& name, const ObjectPtr<IBaseObject>& value, ISerializer* serializer, bool forUpdate) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    static constexpr const char* AuthenticationMethodPropertyName = "AuthenticationMethod";
    static constexpr const char* SuppliedSecretPropertyName = "SuppliedSecret";

    void initProperties(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors);

    // Shared tail of `setPropertyValue`/`setPropertySelectionValue`/`setProtectedPropertyValue`, run after the
    // write itself already succeeded: auto-clears an incompatible "SuppliedSecret" on an "AuthenticationMethod"
    // write.
    ErrCode onPropertyValueChanged(const StringPtr& name);

    void clearSuppliedSecretIfIncompatible(const CredentialDescriptorPtr& selectedDescriptor);

    // Structural check: does `secret` have exactly the property names `selectedDescriptor.createEmptySecret()`
    // would produce? The blessed workflow is to build from that template and fill it in, but this doesn't
    // check provenance, only shape.
    static bool IsSuppliedSecretShapeValid(const PropertyObjectPtr& secret, const CredentialDescriptorPtr& selectedDescriptor);

    static DictPtr<IString, ICredentialDescriptor> ToCredentialDescriptorDict(const ListPtr<IStruct>& candidates);
};

END_NAMESPACE_OPENDAQ
