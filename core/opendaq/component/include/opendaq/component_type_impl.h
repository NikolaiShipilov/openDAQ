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
#include <opendaq/component_type.h>
#include <coretypes/string_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/struct_impl.h>
#include <coretypes/struct_type_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <opendaq/module_info_ptr.h>
#include <opendaq/component_type_private.h>
#include <opendaq/credential_descriptor_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    inline bool ComponentTypeStructHasField(const StructTypePtr& type, const StringPtr& fieldName)
    {
        for (const auto& name : type.getFieldNames())
        {
            if (name == fieldName)
                return true;
        }
        return false;
    }
}

template <typename Intf = IComponentType, typename... Interfaces>
class GenericComponentTypeImpl : public GenericStructImpl<Intf, IStruct, IComponentTypePrivate, Interfaces...>
{
public:
    explicit GenericComponentTypeImpl(const StructTypePtr& type,
                                      const StringPtr& id,
                                      const StringPtr& name,
                                      const StringPtr& description,
                                      const PropertyObjectPtr& defaultConfig,
                                      const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods = nullptr,
                                      const StringPtr& defaultAuthenticationMethodId = nullptr);

    explicit GenericComponentTypeImpl(const StructTypePtr& type,
                                      const StringPtr& id,
                                      const StringPtr& name,
                                      const StringPtr& description,
                                      const StringPtr& prefix,
                                      const PropertyObjectPtr& defaultConfig,
                                      const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods = nullptr,
                                      const StringPtr& defaultAuthenticationMethodId = nullptr);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultConfig(IPropertyObject** defaultConfig) override;
    ErrCode INTERFACE_FUNC getModuleInfo(IModuleInfo** moduleInfo) override;
    ErrCode INTERFACE_FUNC getSupportedAuthenticationMethods(IDict** descriptors) override;
    ErrCode INTERFACE_FUNC getDefaultAuthenticationMethodId(IString** defaultAuthenticationMethodId) override;

    // IComponentTypePrivate
    ErrCode INTERFACE_FUNC setModuleInfo(IModuleInfo* info) override;

private:
    // Only sets "SupportedAuthenticationMethods"/"DefaultAuthenticationMethodId" when `type` actually
    // declares them as Struct fields - not every sort does (e.g. Server/FunctionBlock don't) - since the
    // base `GenericStructImpl` constructor used here does not validate `fields` against `type`'s declared
    // shape, a mismatch would otherwise silently leave the Struct's own field list out of sync with its
    // stored `fields`.
    static DictPtr<IString, IBaseObject> BuildFields(const StructTypePtr& type,
                                                      const StringPtr& id,
                                                      const StringPtr& name,
                                                      const StringPtr& description,
                                                      const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods,
                                                      const StringPtr& defaultAuthenticationMethodId);
    static DictPtr<IString, IBaseObject> BuildFields(const StructTypePtr& type,
                                                      const StringPtr& id,
                                                      const StringPtr& name,
                                                      const StringPtr& description,
                                                      const StringPtr& prefix,
                                                      const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods,
                                                      const StringPtr& defaultAuthenticationMethodId);

protected:
    StringPtr id;
    StringPtr name;
    StringPtr description;
    StringPtr prefix;
    PropertyObjectPtr defaultConfig;
    ModuleInfoPtr moduleInfo;
    DictPtr<IString, ICredentialDescriptor> supportedAuthenticationMethods;
    StringPtr defaultAuthenticationMethodId;
};

template <class Intf, class... Interfaces>
GenericComponentTypeImpl<Intf, Interfaces...>::GenericComponentTypeImpl(const StructTypePtr& type,
                                                                        const StringPtr& id,
                                                                        const StringPtr& name,
                                                                        const StringPtr& description,
                                                                        const PropertyObjectPtr& defaultConfig,
                                                                        const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods,
                                                                        const StringPtr& defaultAuthenticationMethodId)
    : GenericStructImpl<Intf, IStruct, IComponentTypePrivate, Interfaces...>(
          type, BuildFields(type, id, name, description, supportedAuthenticationMethods, defaultAuthenticationMethodId))
    , id(id)
    , name(name)
    , description(description)
    , prefix("")
    , defaultConfig(defaultConfig)
    , supportedAuthenticationMethods(supportedAuthenticationMethods)
    , defaultAuthenticationMethodId(defaultAuthenticationMethodId)
{
}

template <typename Intf, typename... Interfaces>
GenericComponentTypeImpl<Intf, Interfaces...>::GenericComponentTypeImpl(const StructTypePtr& type,
                                                                        const StringPtr& id,
                                                                        const StringPtr& name,
                                                                        const StringPtr& description,
                                                                        const StringPtr& prefix,
                                                                        const PropertyObjectPtr& defaultConfig,
                                                                        const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods,
                                                                        const StringPtr& defaultAuthenticationMethodId)
    : GenericStructImpl<Intf, IStruct, IComponentTypePrivate, Interfaces...>(
          type, BuildFields(type, id, name, description, prefix, supportedAuthenticationMethods, defaultAuthenticationMethodId))
    , id(id)
    , name(name)
    , description(description)
    , prefix(prefix)
    , defaultConfig(defaultConfig)
    , supportedAuthenticationMethods(supportedAuthenticationMethods)
    , defaultAuthenticationMethodId(defaultAuthenticationMethodId)
{
}

template <class Intf, class... Interfaces>
DictPtr<IString, IBaseObject> GenericComponentTypeImpl<Intf, Interfaces...>::BuildFields(
    const StructTypePtr& type,
    const StringPtr& id,
    const StringPtr& name,
    const StringPtr& description,
    const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods,
    const StringPtr& defaultAuthenticationMethodId)
{
    auto fields = Dict<IString, IBaseObject>({{"Id", id}, {"Name", name}, {"Description", description}});
    if (detail::ComponentTypeStructHasField(type, "SupportedAuthenticationMethods"))
        fields.set("SupportedAuthenticationMethods", supportedAuthenticationMethods);
    if (detail::ComponentTypeStructHasField(type, "DefaultAuthenticationMethodId"))
        fields.set("DefaultAuthenticationMethodId", defaultAuthenticationMethodId);
    return fields;
}

template <class Intf, class... Interfaces>
DictPtr<IString, IBaseObject> GenericComponentTypeImpl<Intf, Interfaces...>::BuildFields(
    const StructTypePtr& type,
    const StringPtr& id,
    const StringPtr& name,
    const StringPtr& description,
    const StringPtr& prefix,
    const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods,
    const StringPtr& defaultAuthenticationMethodId)
{
    auto fields = Dict<IString, IBaseObject>({{"Id", id}, {"Name", name}, {"Description", description}, {"Prefix", prefix}});
    if (detail::ComponentTypeStructHasField(type, "SupportedAuthenticationMethods"))
        fields.set("SupportedAuthenticationMethods", supportedAuthenticationMethods);
    if (detail::ComponentTypeStructHasField(type, "DefaultAuthenticationMethodId"))
        fields.set("DefaultAuthenticationMethodId", defaultAuthenticationMethodId);
    return fields;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::createDefaultConfig(IPropertyObject** defaultConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultConfig);

    if (this->defaultConfig.assigned())
        return this->defaultConfig.template asPtr<IPropertyObjectInternal>()->clone(defaultConfig);

    *defaultConfig = PropertyObject().detach();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getSupportedAuthenticationMethods(IDict** descriptors)
{
    OPENDAQ_PARAM_NOT_NULL(descriptors);

    *descriptors = this->supportedAuthenticationMethods.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getDefaultAuthenticationMethodId(IString** defaultAuthenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(defaultAuthenticationMethodId);

    *defaultAuthenticationMethodId = this->defaultAuthenticationMethodId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename Intf, typename... Interfaces>
inline ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::setModuleInfo(IModuleInfo* info)
{
    this->moduleInfo = info;

    return OPENDAQ_SUCCESS;
}

template <typename Intf, typename... Interfaces>
inline ErrCode INTERFACE_FUNC GenericComponentTypeImpl<Intf, Interfaces...>::getModuleInfo(IModuleInfo** moduleInfo)
{
    OPENDAQ_PARAM_NOT_NULL(moduleInfo);

    *moduleInfo = this->moduleInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
