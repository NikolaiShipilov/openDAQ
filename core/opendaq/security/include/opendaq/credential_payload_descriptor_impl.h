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
#include <opendaq/credential_payload_descriptor.h>
#include <coretypes/impl.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/boolean_factory.h>
#include <coretypes/struct_impl.h>
#include <coretypes/serializable.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief `IStruct` impl for the parameter set nested inside a `CredentialPayloadDescriptorImpl`. Still
 * overrides `serialize`/`getSerializeId`/`Deserialize` (rather than relying on `GenericStructImpl<IStruct>`'s
 * own generic versions) because the generic `Deserialize` requires `context` to directly expose
 * `ITypeManager` via `queryInterface` - which does not hold for the context used while reloading a
 * component (verified: a real device reload fails with "Context does not implement ITypeManager interface
 * for Struct deserialization" otherwise). This one instead reconstructs directly from whichever of
 * `"Keys"`/`"Hidden"`/neither is present in the serialized data, with no type manager involved at all.
 */
class CredentialPayloadDescriptorParametersImpl final : public GenericStructImpl<IStruct>
{
public:
    CredentialPayloadDescriptorParametersImpl(const StructTypePtr& structType, const DictPtr<IString, IBaseObject>& fields);

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

/*!
 * @brief `ICredentialPayloadDescriptor` impl for all three payload formats. One non-templated class - the
 * only thing that genuinely varies per format is which public constructor is used (and so which fields the
 * built `"Parameters"` Struct has); `format` itself is then just a stored, runtime-checked member, not a
 * template parameter.
 */
class CredentialPayloadDescriptorImpl final : public GenericStructImpl<ICredentialPayloadDescriptor, IStruct>
{
public:
    // `id`/`keys`/`description` are raw interface pointers, not smart pointers, so these three overloads
    // stay unambiguous: flattening away the old per-format template means they now share one overload set,
    // and `ObjectPtr`'s generic converting constructor (accepting any interface pointer via a runtime
    // `queryInterface`) would otherwise make a single smart-pointer-typed argument an equally-ranked
    // candidate for all three. Raw pointers of unrelated interfaces have no such implicit conversion between
    // them, so each argument is only ever viable for its own overload. `typeManager`, if assigned and it
    // already has the format's struct type registered (see `RegisterCredentialPayloadDescriptorTypes`), is
    // used to build the descriptor - and its `createDefaultPayload()` stub for `String`/`FilePath` - with
    // the registered types instead of building unregistered ones locally.

    // KeyValuePairs
    CredentialPayloadDescriptorImpl(IString* id, IDict* keys, IString* description, ITypeManager* typeManager = nullptr);
    // String
    CredentialPayloadDescriptorImpl(IString* id, IString* description, Bool hidden, ITypeManager* typeManager = nullptr);
    // FilePath
    CredentialPayloadDescriptorImpl(IString* id, IString* description, ITypeManager* typeManager = nullptr);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) override;
    ErrCode INTERFACE_FUNC getParameters(IStruct** parameters) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultPayload(IPropertyObject** payload) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    // Same raw-pointer disambiguation rationale as the public constructors above - this is an overloaded
    // set too.
    static DictPtr<IString, IBaseObject> BuildFields(IString* id, IDict* keys, IString* description, const TypeManagerPtr& typeManager);
    static DictPtr<IString, IBaseObject> BuildFields(IString* id, IString* description, Bool hidden, const TypeManagerPtr& typeManager);
    static DictPtr<IString, IBaseObject> BuildFields(IString* id, IString* description, const TypeManagerPtr& typeManager);

    CredentialPayloadDescriptorImpl(CredentialPayloadFormat format,
                                    const StructTypePtr& structType,
                                    const DictPtr<IString, IBaseObject>& fields,
                                    const TypeManagerPtr& typeManager);

    CredentialPayloadFormat format;
    TypeManagerPtr typeManager;
};

// The class factory macros in the .cpp token-paste `<FactoryName>Impl` as the type to construct (e.g.
// `KeyValuePayloadDescriptorImpl`) - these aliases just point each of the three expected names at the one
// real class above, picking the matching constructor overload by argument list.
using KeyValuePayloadDescriptorImpl = CredentialPayloadDescriptorImpl;
using StringPayloadDescriptorImpl = CredentialPayloadDescriptorImpl;
using FilePathPayloadDescriptorImpl = CredentialPayloadDescriptorImpl;

END_NAMESPACE_OPENDAQ
