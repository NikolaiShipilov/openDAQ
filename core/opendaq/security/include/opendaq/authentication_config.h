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
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 */

DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfig, IBaseObject)
{
    /*!
     * @brief Gets the id of the payload (secret shape) this authentication method produces.
     * @param[out] payloadId The payload id.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialPayloadId(IString** payloadId) = 0;

    /*!
     * @brief Gets the descriptor of the payload this authentication method produces.
     * @param[out] descriptor The payload descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) = 0;

    /*!
     * @brief Gets additional, method-specific configuration.
     * @param[out] config The configuration property object.
     */
    virtual ErrCode INTERFACE_FUNC getConfig(IPropertyObject** config) = 0;
};

END_NAMESPACE_OPENDAQ
