set(BASE_NAME security)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_CredentialProvider credential_provider.h)
    rtgen(SRC_CredentialPayload credential_payload.h)
    rtgen(SRC_CredentialRequest credential_request.h)
    rtgen(SRC_CredentialRequestBuilder credential_request_builder.h)
    rtgen(SRC_AuthenticationConfig authentication_config.h)
    rtgen(SRC_CredentialPayloadDescriptor credential_payload_descriptor.h)

    set(SRC_PublicHeaders_Component_Generated
        ${SRC_CredentialProvider_PublicHeaders}
        ${SRC_CredentialPayload_PublicHeaders}
        ${SRC_CredentialRequest_PublicHeaders}
        ${SRC_CredentialRequestBuilder_PublicHeaders}
        ${SRC_AuthenticationConfig_PublicHeaders}
        ${SRC_CredentialPayloadDescriptor_PublicHeaders}
        PARENT_SCOPE
    )

    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_CredentialProvider_PrivateHeaders}
        ${SRC_CredentialPayload_PrivateHeaders}
        ${SRC_CredentialRequest_PrivateHeaders}
        ${SRC_CredentialRequestBuilder_PrivateHeaders}
        ${SRC_AuthenticationConfig_PrivateHeaders}
        ${SRC_CredentialPayloadDescriptor_PrivateHeaders}
        PARENT_SCOPE
    )

    set(SRC_Cpp_Component_Generated
        ${SRC_CredentialProvider_Cpp}
        ${SRC_CredentialPayload_Cpp}
        ${SRC_CredentialRequest_Cpp}
        ${SRC_CredentialRequestBuilder_Cpp}
        ${SRC_AuthenticationConfig_Cpp}
        ${SRC_CredentialPayloadDescriptor_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "security/include/opendaq")
    set(SDK_SRC_DIR "security/src")

    source_group("security//credentials" FILES 
        ${SDK_HEADERS_DIR}/credential_provider.h
        ${SDK_HEADERS_DIR}/credential_provider_factory.h
        ${SDK_HEADERS_DIR}/credential_payload.h
        ${SDK_HEADERS_DIR}/credential_payload_factory.h
        ${SDK_HEADERS_DIR}/credential_request.h
        ${SDK_HEADERS_DIR}/credential_request_factory.h
        ${SDK_HEADERS_DIR}/credential_request_impl.h
        ${SDK_HEADERS_DIR}/credential_request_builder.h
        ${SDK_HEADERS_DIR}/credential_request_builder_impl.h
        ${SDK_HEADERS_DIR}/key_value_credential_payload_impl.h
        ${SDK_HEADERS_DIR}/string_credential_payload_impl.h
        ${SDK_HEADERS_DIR}/cmd_line_credential_provider_impl.h
        ${SDK_HEADERS_DIR}/authentication_config.h
        ${SDK_HEADERS_DIR}/authentication_config_factory.h
        ${SDK_HEADERS_DIR}/authentication_config_impl.h
        ${SDK_HEADERS_DIR}/credential_payload_descriptor.h
        ${SDK_HEADERS_DIR}/credential_payload_descriptor_factory.h
        ${SDK_HEADERS_DIR}/credential_payload_descriptor_impl.h
        ${SDK_SRC_DIR}/credential_request_impl.cpp
        ${SDK_SRC_DIR}/key_value_credential_payload_impl.cpp
        ${SDK_SRC_DIR}/string_credential_payload_impl.cpp
        ${SDK_SRC_DIR}/cmd_line_credential_provider_impl.cpp
        ${SDK_SRC_DIR}/credential_request_builder_impl.cpp
        ${SDK_SRC_DIR}/credential_payload_descriptor_impl.cpp
        ${SDK_SRC_DIR}/authentication_config_impl.cpp
    )
endfunction()

set(SRC_PublicHeaders_Component
    credential_provider.h
    credential_provider_factory.h
    credential_payload.h
    credential_payload_factory.h
    credential_request.h
    credential_request_builder.h
    credential_request_factory.h
    authentication_config.h
    authentication_config_factory.h
    credential_payload_descriptor.h
    credential_payload_descriptor_factory.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    credential_request_builder_impl.h
    credential_request_impl.h
    cmd_line_credential_provider_impl.h
    key_value_credential_payload_impl.h
    string_credential_payload_impl.h
    credential_payload_descriptor_impl.h
    authentication_config_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component
    credential_request_impl.cpp
    key_value_credential_payload_impl.cpp
    string_credential_payload_impl.cpp
    cmd_line_credential_provider_impl.cpp
    credential_request_builder_impl.cpp
    credential_payload_descriptor_impl.cpp
    authentication_config_impl.cpp
    PARENT_SCOPE
)
