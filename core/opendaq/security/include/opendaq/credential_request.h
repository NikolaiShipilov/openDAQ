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
#include <coreobjects/property_object_ptr.h>
#include <opendaq/credential_payload_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 * [interfaceLibrary(ICredentialPayloadDescriptor, "opendaq")]
 * [interfaceSmartPtr(ICredentialPayloadDescriptor, CredentialPayloadDescriptorPtr, "<opendaq/credential_payload_descriptor_ptr.h>")]
 * [interfaceLibrary(IComponentType, "opendaq")]
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<opendaq/component_type_ptr.h>")]
 */

struct ICredentialRequestBuilder;
struct IComponentType;

/*!
 * @brief Carries the non-secret details of a credential request handed to `ICredentialProvider::requestCredentials`
 * when authentication is required for a connection attempt.
 *
 * Built via `ICredentialRequestBuilder`, or reconstructed on load. Never carries the actual secrets - only
 * enough context (the component type, connection details, and the negotiated payload's id and descriptor)
 * for the provider to determine how to provide the secrets.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialRequest, IBaseObject)
{
    /*!
     * @brief Gets the type of the component the request is for.
     * @param[out] componentType The component type.
     */
    // [templateType(componentType, IComponentType)]
    virtual ErrCode INTERFACE_FUNC getComponentType(IComponentType** componentType) = 0;

    /*!
     * @brief Gets the connection string used for the connection attempt this request was formed for.
     * @param[out] connectionString The connection string.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

    /*!
     * @brief Gets additional metadata describing the request, for the credential provider to present to
     * the user (e.g. device type name/id/description).
     * @param[out] metaData The metadata property object.
     */
    virtual ErrCode INTERFACE_FUNC getMetaData(IPropertyObject** metaData) = 0;

    /*!
     * @brief Gets the manufacturer of the device the request is for.
     * @param[out] manufacturer The device manufacturer.
     */
    virtual ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) = 0;

    /*!
     * @brief Gets the serial number of the device the request is for.
     * @param[out] serialNumber The device serial number.
     */
    virtual ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) = 0;

    /*!
     * @brief Gets the id of the negotiated payload - obtained from `IAuthenticationConfig` - serialized on save & replayed on load.
     * @param[out] payloadId The payload id.
     */
    virtual ErrCode INTERFACE_FUNC getPayloadId(IString** payloadId) = 0;

    /*!
     * @brief Gets the descriptor of the payload the provider must provide - serialized on save or re-attached from the
     * device type on load.
     * @param[out] descriptor The payload descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) = 0;
};

/*!
 * @brief Creates a `CredentialRequest` from a `ICredentialRequestBuilder`.
 */
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CredentialRequestFromBuilder, ICredentialRequest,
    ICredentialRequestBuilder*, builder
)

END_NAMESPACE_OPENDAQ
