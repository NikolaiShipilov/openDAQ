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
#include <coretypes/string_ptr.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief The shape of the secret(s) carried by a credential payload.
 */
enum class CredentialPayloadFormat : EnumType
{
    KeyValuePairs,  ///< N string pairs - e.g. UserName / Password.
};

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 */

/*!
 * @brief Describes the details of the payload required for an authentication method used by the module and produced by credential provider.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialPayloadDescriptor, IBaseObject)
{
    /*!
     * @brief Gets the format of the described payload.
     * @param[out] format The payload format.
     */
    virtual ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) = 0;

    /*!
     * @brief Gets the format's standard parameter set.
     * @param[out] parameters The parameters.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IPropertyObject** parameters) = 0;

    /*!
     * @brief Gets the description of the payload, for the user. States how the module interpretes it,
     * e.g. "PIN-code", "username and password", "Raw bytes of the SSH private key", "Path to file containing the SSH private key".
     * @param[out] description The payload description.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor,
    IList*, keys, IString*, description
)

END_NAMESPACE_OPENDAQ
