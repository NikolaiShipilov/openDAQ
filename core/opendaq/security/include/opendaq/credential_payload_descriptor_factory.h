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
#include <opendaq/credential_payload_descriptor_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/boolean_factory.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/ctutils.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Name of the `IPropertyObjectClass` (see `CredentialSecretPayloadClass`) backing the default
 * payload template `createDefaultPayload()` builds for `String`/`FilePath`-format descriptors - both
 * formats need only a single `"Secret"` string property, so they share this one class rather than each
 * descriptor instance building its own ad hoc property object.
 */
inline constexpr const char* CredentialSecretPayloadClassName = "CredentialSecretPayload";

/*!
 * @brief The `IPropertyObjectClass` backing the default payload template for `String`/`FilePath`-format
 * descriptors - a single `"Secret"` string property.
 */
inline PropertyObjectClassPtr CredentialSecretPayloadClass()
{
    return PropertyObjectClassBuilder(CredentialSecretPayloadClassName).addProperty(StringProperty("Secret", "")).build();
}

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format descriptor's nested `"Parameters"` field - a
 * single `"Keys"` dict field. The single source of truth for this shape - `CredentialPayloadDescriptorImpl`
 * and `Context` (see `RegisterCredentialPayloadDescriptorTypes`) both build it from here, never redefine it.
 */
inline StructTypePtr KeyValuePayloadDescriptorParametersStructType()
{
    return StructType("KeyValuePayloadDescriptorParameters", List<IString>("Keys"), List<IType>(SimpleType(ctDict)));
}

/*!
 * @brief The `IStructType` backing a `String`-format descriptor's nested `"Parameters"` field - a single
 * `"Hidden"` bool field.
 */
inline StructTypePtr StringPayloadDescriptorParametersStructType()
{
    return StructType("StringPayloadDescriptorParameters", List<IString>("Hidden"), List<IType>(SimpleType(ctBool)));
}

/*!
 * @brief The `IStructType` backing a `FilePath`-format descriptor's nested `"Parameters"` field - no
 * fields at all.
 */
inline StructTypePtr FilePathPayloadDescriptorParametersStructType()
{
    return StructType("FilePathPayloadDescriptorParameters", List<IString>(), List<IType>());
}

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format `CredentialPayloadDescriptor`.
 */
inline StructTypePtr KeyValuePayloadDescriptorStructType()
{
    return StructType("KeyValuePayloadDescriptor",
                      List<IString>("Id", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), KeyValuePayloadDescriptorParametersStructType()));
}

/*!
 * @brief The `IStructType` backing a `String`-format `CredentialPayloadDescriptor`.
 */
inline StructTypePtr StringPayloadDescriptorStructType()
{
    return StructType("StringPayloadDescriptor",
                      List<IString>("Id", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), StringPayloadDescriptorParametersStructType()));
}

/*!
 * @brief The `IStructType` backing a `FilePath`-format `CredentialPayloadDescriptor`.
 */
inline StructTypePtr FilePathPayloadDescriptorStructType()
{
    return StructType("FilePathPayloadDescriptor",
                      List<IString>("Id", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), FilePathPayloadDescriptorParametersStructType()));
}

/*!
 * @brief Registers the three payload formats' backing `IStructType`s (and their nested `Parameters` struct
 * types), plus the `CredentialSecretPayloadClass` used for `createDefaultPayload()`, with `typeManager` -
 * covers every `CredentialPayloadDescriptor`, standard or module-custom, since all descriptors of a given
 * format share that one format's struct type. Called once by `Context` itself, before any module is loaded
 * (see `ContextImpl::registerOpenDaqTypes`) - the same "framework registers its well-known types up front"
 * pattern already used there for `ComponentStatusType`/`ConnectionStatusType`/etc. Idempotent (tolerates
 * `OPENDAQ_ERR_ALREADYEXISTS`), so it's harmless to call again from anywhere that isn't certain
 * registration already happened.
 * @param typeManager The type manager to register the payload descriptor types with.
 */
inline void RegisterCredentialPayloadDescriptorTypes(const TypeManagerPtr& typeManager)
{
    for (const auto& type : {KeyValuePayloadDescriptorStructType(),
                             KeyValuePayloadDescriptorParametersStructType(),
                             StringPayloadDescriptorStructType(),
                             StringPayloadDescriptorParametersStructType(),
                             FilePathPayloadDescriptorStructType(),
                             FilePathPayloadDescriptorParametersStructType()})
    {
        checkErrorInfoExcept(typeManager->addType(type), OPENDAQ_ERR_ALREADYEXISTS);
    }

    checkErrorInfoExcept(typeManager->addType(CredentialSecretPayloadClass()), OPENDAQ_ERR_ALREADYEXISTS);
}

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `KeyValuePairs`-format payload.
 * @param id The id that uniquely identifies this authentication method within the module that offers it.
 * @param keys The expected keys, mapped to whether the corresponding value should be hidden as it is
 * entered (e.g. `{"UserName": False, "Password": True}`).
 * @param description A human-readable description of the payload, for the user.
 * @param typeManager If assigned and it already has a `"KeyValuePayloadDescriptor"` type registered (see
 * `RegisterCredentialPayloadDescriptorTypes`), the descriptor is built with that registered type instead of
 * building its own - the usual case once `Context` has registered it up front. Left unassigned (the
 * default), the descriptor builds its own, unregistered type, exactly as before.
 */
inline CredentialPayloadDescriptorPtr KeyValuePayloadDescriptor(const StringPtr& id,
                                                                const DictPtr<IString, IBoolean>& keys,
                                                                const StringPtr& description,
                                                                const TypeManagerPtr& typeManager = nullptr)
{
    CredentialPayloadDescriptorPtr obj(KeyValuePayloadDescriptor_Create(id, keys, description, typeManager));
    return obj;
}

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `String`-format payload - a single secret,
 * e.g. a PIN, token, or API key.
 * @param id The id that uniquely identifies this authentication method within the module that offers it.
 * @param description A human-readable description of the payload, for the user.
 * @param hidden Whether the secret should be hidden as it is entered.
 * @param typeManager If assigned and it already has a `"StringPayloadDescriptor"` type registered (see
 * `RegisterCredentialPayloadDescriptorTypes`), the descriptor is built with that registered type instead of
 * building its own - the usual case once `Context` has registered it up front. Left unassigned (the
 * default), the descriptor builds its own, unregistered type, exactly as before.
 */
inline CredentialPayloadDescriptorPtr StringPayloadDescriptor(const StringPtr& id,
                                                               const StringPtr& description,
                                                               Bool hidden = True,
                                                               const TypeManagerPtr& typeManager = nullptr)
{
    CredentialPayloadDescriptorPtr obj(StringPayloadDescriptor_Create(id, description, hidden, typeManager));
    return obj;
}

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `FilePath`-format payload - a single secret
 * stating that the secret is a path to a file (e.g. a private key) rather than the value itself.
 * @param id The id that uniquely identifies this authentication method within the module that offers it.
 * @param description A human-readable description of the payload, for the user.
 * @param typeManager If assigned and it already has a `"FilePathPayloadDescriptor"` type registered (see
 * `RegisterCredentialPayloadDescriptorTypes`), the descriptor is built with that registered type instead of
 * building its own - the usual case once `Context` has registered it up front. Left unassigned (the
 * default), the descriptor builds its own, unregistered type, exactly as before.
 */
inline CredentialPayloadDescriptorPtr FilePathPayloadDescriptor(const StringPtr& id,
                                                                 const StringPtr& description,
                                                                 const TypeManagerPtr& typeManager = nullptr)
{
    CredentialPayloadDescriptorPtr obj(FilePathPayloadDescriptor_Create(id, description, typeManager));
    return obj;
}

/*!
 * @brief Ids of the three standard authentication methods below - shared, well-known payload ids every
 * module can build the exact same descriptor for, instead of each one inventing its own shape/id for the
 * same method.
 */
inline constexpr const char* StandardUserNamePasswordPayloadId = "UserNamePassword";
inline constexpr const char* StandardPinPayloadId = "Pin";
inline constexpr const char* StandardPrivateKeyFilePayloadId = "PrivateKeyFile";

/*!
 * @brief The standard `UserName`/`Password` authentication method's `CredentialPayloadDescriptor` - a
 * `KeyValuePairs`-format payload with the password hidden as typed.
 * @param typeManager See `KeyValuePayloadDescriptor`.
 */
inline CredentialPayloadDescriptorPtr StandardUserNamePasswordPayloadDescriptor(const TypeManagerPtr& typeManager = nullptr)
{
    return KeyValuePayloadDescriptor(
        StandardUserNamePasswordPayloadId, Dict<IString, IBoolean>({{"UserName", False}, {"Password", True}}), "Username and password", typeManager);
}

/*!
 * @brief The standard PIN authentication method's `CredentialPayloadDescriptor` - a `String`-format
 * payload, hidden as typed.
 * @param typeManager See `StringPayloadDescriptor`.
 */
inline CredentialPayloadDescriptorPtr StandardPinPayloadDescriptor(const TypeManagerPtr& typeManager = nullptr)
{
    return StringPayloadDescriptor(StandardPinPayloadId, "PIN code", True, typeManager);
}

/*!
 * @brief The standard private-key-file authentication method's `CredentialPayloadDescriptor` - a
 * `FilePath`-format payload.
 * @param typeManager See `FilePathPayloadDescriptor`.
 */
inline CredentialPayloadDescriptorPtr StandardPrivateKeyFilePayloadDescriptor(const TypeManagerPtr& typeManager = nullptr)
{
    return FilePathPayloadDescriptor(StandardPrivateKeyFilePayloadId, "Path to the PEM-encoded private key file", typeManager);
}

END_NAMESPACE_OPENDAQ
