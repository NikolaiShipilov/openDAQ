#include <gtest/gtest.h>
#include <opendaq/authentication_method_factory.h>
#include <opendaq/authentication_method_internal_ptr.h>

using AuthenticationMethodTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(AuthenticationMethodTest, FactoryNoFields)
{
    const AuthenticationMethodPtr method = AuthenticationMethod("AnonymousTest", Dict<IString, ICredentialField>(), "No credentials required");

    ASSERT_EQ(method.getId(), "AnonymousTest");
    ASSERT_EQ(method.getFields().getCount(), 0u);
    ASSERT_EQ(method.getDescription(), "No credentials required");
}

TEST_F(AuthenticationMethodTest, FactoryThrowsOnUnassignedParams)
{
    ASSERT_THROW(AuthenticationMethod(nullptr, Dict<IString, ICredentialField>(), "No credentials required"), InvalidParameterException);
    ASSERT_THROW(AuthenticationMethod("AnonymousTest", nullptr, "No credentials required"), InvalidParameterException);
    ASSERT_THROW(AuthenticationMethod("AnonymousTest", Dict<IString, ICredentialField>(), nullptr), InvalidParameterException);
}

TEST_F(AuthenticationMethodTest, FactoryWithFields)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")},
         {"Password", CredentialField("Password", CredentialFieldKind::Secret, "Password")}});

    const AuthenticationMethodPtr method = AuthenticationMethod("UserNamePasswordTest", fields, "Username and password");

    ASSERT_EQ(method.getId(), "UserNamePasswordTest");
    ASSERT_EQ(method.getDescription(), "Username and password");
    ASSERT_EQ(method.getFields().getCount(), 2u);
    ASSERT_EQ(method.getFields().get("UserName").getKind(), CredentialFieldKind::Text);
    ASSERT_EQ(method.getFields().get("Password").getKind(), CredentialFieldKind::Secret);
}

TEST_F(AuthenticationMethodTest, SerializeDeserialize)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"Pin", CredentialField("Pin", CredentialFieldKind::Secret, "PIN code")}});
    const AuthenticationMethodPtr method = AuthenticationMethod("PinTest", fields, "PIN code");

    const auto serializer = JsonSerializer();
    method.serialize(serializer);
    const auto serialized = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const AuthenticationMethodPtr newMethod = deserializer.deserialize(serialized, nullptr, nullptr);

    ASSERT_EQ(newMethod.getId(), "PinTest");
    ASSERT_EQ(newMethod.getDescription(), "PIN code");
    ASSERT_EQ(newMethod.getFields().getCount(), 1u);
    ASSERT_EQ(newMethod.getFields().get("Pin").getKind(), CredentialFieldKind::Secret);

    serializer.reset();
    newMethod.serialize(serializer);
    ASSERT_EQ(serializer.getOutput(), serialized);
}

TEST_F(AuthenticationMethodTest, StandardMethods)
{
    const auto anonymous = StandardAnonymousAuthenticationMethod();
    ASSERT_EQ(anonymous.getId(), StandardAnonymousId);
    ASSERT_EQ(anonymous.getFields().getCount(), 0u);

    const auto userNamePassword = StandardUserNamePasswordAuthenticationMethod();
    const auto userNamePasswordFields = userNamePassword.getFields();
    ASSERT_EQ(userNamePassword.getId(), StandardUserNamePasswordId);
    ASSERT_EQ(userNamePasswordFields.getCount(), 2u);
    ASSERT_EQ(userNamePasswordFields.get("UserName").getKind(), CredentialFieldKind::Text);
    ASSERT_EQ(userNamePasswordFields.get("Password").getKind(), CredentialFieldKind::Secret);

    const auto pin = StandardPinAuthenticationMethod();
    const auto pinFields = pin.getFields();
    ASSERT_EQ(pin.getId(), StandardPinId);
    ASSERT_EQ(pinFields.getCount(), 1u);
    ASSERT_EQ(pinFields.get("Pin").getKind(), CredentialFieldKind::Secret);

    const auto privateKeyFile = StandardPrivateKeyFileAuthenticationMethod();
    const auto privateKeyFileFields = privateKeyFile.getFields();
    ASSERT_EQ(privateKeyFile.getId(), StandardPrivateKeyFileId);
    ASSERT_EQ(privateKeyFileFields.getCount(), 1u);
    const auto privateKeyField = privateKeyFileFields.get("PrivateKeyFilePath");
    ASSERT_EQ(privateKeyField.getKind(), CredentialFieldKind::FilePath);
    ASSERT_EQ(privateKeyField.getMetadata().get("Extensions"), "pem,key");
}

TEST_F(AuthenticationMethodTest, CredentialAcceptionThrowsOnUnassignedParams)
{
    ASSERT_THROW(StandardUserNamePasswordAuthenticationMethod().asPtr<IAuthenticationMethodInternal>().acceptsCredentials(nullptr),
                 ArgumentNullException);
}

TEST_F(AuthenticationMethodTest, CredentialAcceptionThrowsOnMethodWithNoFields)
{
    const auto method = StandardAnonymousAuthenticationMethod().asPtr<IAuthenticationMethodInternal>();

    ASSERT_THROW(method.acceptsCredentials(Dict<IString, IString>()), NotSupportedException);
    ASSERT_THROW(method.acceptsCredentials(Dict<IString, IString>({{"Whatever", "value"}})), NotSupportedException);
}

TEST_F(AuthenticationMethodTest, CredentialAcceptionByStandardMethod)
{
    const auto method = StandardUserNamePasswordAuthenticationMethod().asPtr<IAuthenticationMethodInternal>();

    ASSERT_FALSE(method.acceptsCredentials(Dict<IString, IString>({{"Password", "qwerty"}}))); // Missing Required Value
    ASSERT_FALSE(method.acceptsCredentials(Dict<IString, IString>({{"UserName", ""}, {"Password", "qwerty"}}))); // Empty Required Value
    ASSERT_TRUE(method.acceptsCredentials(Dict<IString, IString>({{"UserName", "john"}, {"Password", "qwerty"}}))); // All Required Fields Present
}

TEST_F(AuthenticationMethodTest, CredentialAcceptionNonRequiredFieldIgnored)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")},
         {"Field", CredentialField("Field", CredentialFieldKind::Text, "Field", Dict<IString, IString>(), False)}});
    const auto method = AuthenticationMethod("CustomTest", fields, "Custom method").asPtr<IAuthenticationMethodInternal>();

    ASSERT_TRUE(method.acceptsCredentials(Dict<IString, IString>({{"UserName", "john"}})));
}

// A method whose fields are all optional accepts empty credential.
TEST_F(AuthenticationMethodTest, CredentialAcceptionAllFieldsOptional)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"Field", CredentialField("Field", CredentialFieldKind::Text, "Field", Dict<IString, IString>(), False)}});
    const auto method = AuthenticationMethod("AllOptionalTest", fields, "All fields optional").asPtr<IAuthenticationMethodInternal>();

    ASSERT_TRUE(method.acceptsCredentials(Dict<IString, IString>()));
    ASSERT_TRUE(method.acceptsCredentials(Dict<IString, IString>({{"Field", ""}})));
}

END_NAMESPACE_OPENDAQ
