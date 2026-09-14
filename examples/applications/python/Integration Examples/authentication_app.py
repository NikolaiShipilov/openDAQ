##
# Scaffolding for investigating the credential / authentication API from Python.
#
# Discovers every available device (local and remote) and, for each one, calls
# "process_authentication" - the hook standing in for what an application does when the user
# clicks "Add authenticated device" in a GUI: take the authentication config the device
# offers for that device type, select a method on it, and call
# "instance.add_authenticated_device(connection_string, config, authentication_config)".
#
# The instance is built with credential providers registered, since the authenticated path
# needs at least one provider able to supply the format the selected method asks for.
# CmdLineCredentialProvider prompts on stdin; FileCredentialProvider reads the secret from a
# file. Registration order matters: the config's "CredentialProviderId" defaults to the first
# registered provider compatible with the currently selected method.
#
# The device type itself carries no authentication data - a module declares the methods it
# supports keyed by the type's id, and they are reached through
# "instance.create_default_authentication_config(type_id)". That returns one self-contained
# config whose "AuthenticationMethod" selection property lists every supported method;
# switching methods means moving that selection, not fetching a different config.
#
# The credential demo module (DAQMODULES_CREDENTIAL_DEMO_MODULE) is the module that actually
# supports authentication: it advertises "Credential demo device" with the methods
# UserNamePassword (its default; user/pass), Pin (1234), PrivateKeyFile and Anonymous.
##

import opendaq as daq
import os
import sys

py_include = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.append(py_include)

import daq_utils


# Builds the instance the application runs on, with the credential providers a GUI would
# offer registered up front. Providers are never serialized, so a reloaded instance has to
# register its own again.
def build_instance():
    builder = daq.InstanceBuilder()

    try:
        daq.OPENDAQ_MODULES_DIR
    except:
        builder.module_path = '.'
    else:
        builder.module_path = daq.OPENDAQ_MODULES_DIR

    # FileCredentialProvider first: it and CmdLineCredentialProvider both handle the FilePath
    # format, and an authentication config defaults to the first compatible one registered.
    for provider in (daq.FileCredentialProvider(), daq.CmdLineCredentialProvider()):
        builder.add_credential_provider(provider.id, provider)

    return builder.build()


# Finds the device type that would handle a discovered device. The type carried on the device
# info is looked up in the instance's own registry, since that is the copy the module manager
# resolves authentication methods against. Returns None if no type matches.
def resolve_device_type(available_device_types, device_info):
    device_type = device_info.device_type
    if device_type is None:
        return None

    if device_type.id in available_device_types.keys():
        return daq.IDeviceType.cast_from(available_device_types[device_type.id])

    return device_type


# Builds the authentication config for a device type, or returns None if the type does not
# support authentication.
def create_authentication_config(instance, device_type):
    if device_type is None:
        return None

    try:
        return instance.create_default_authentication_config(device_type.id)
    except RuntimeError:
        return None


# Lists the authentication methods an authentication config offers - the candidates of its
# "AuthenticationMethod" selection property, one per method the type's module supports.
def authentication_methods(authentication_config):
    candidates = authentication_config.get_property("AuthenticationMethod").selection_values
    return [daq.ICredentialDescriptor.cast_from(candidate) for candidate in candidates]


# Selects one of the supported methods by authentication method id
def select_authentication_method(authentication_config, authentication_method_id):
    candidates = authentication_config.get_property("AuthenticationMethod").selection_values
    for candidate in candidates:
        if daq.ICredentialDescriptor.cast_from(candidate).authentication_method_id == authentication_method_id:
            authentication_config.set_property_selection_value("AuthenticationMethod", candidate)
            return True

    return False


# Prints what a device offers in terms of authentication - the entry point for working out
# which methods a GUI would put in front of the user, and which provider would serve them.
def print_authentication_support(authentication_config):
    if authentication_config is None:
        daq_utils.print_indented('- Authentication: not supported by this device type', 1)
        return

    descriptors = authentication_methods(authentication_config)
    daq_utils.print_indented('- Authentication methods: ' + ', '.join(d.authentication_method_id for d in descriptors), 1)

    selected = authentication_config.credential_descriptor
    daq_utils.print_indented('- Selected method: ' + selected.authentication_method_id +
                             ' (' + str(selected.format) + ') - ' + selected.description, 1)

    # Absent entirely when no registered provider supports the selected method's format.
    provider_id = authentication_config.credential_provider_id
    daq_utils.print_indented('- Credential provider: ' + (provider_id or '<none compatible>'), 1)


def process_authentication(instance, device_info, device_type, authentication_config):
    authentication_config.set_property_selection_value("CredentialProviderId", "CmdLineCredentialProvider")
    user_pass_available = select_authentication_method(authentication_config, "UserNamePassword")

    if not user_pass_available:
        print("Username and password authentication is not available for this device")

    secret = authentication_config.credential_descriptor.create_empty_secret()
    secret.set_property_value("UserName", "user")
    secret.set_property_value("Password", "pass")
    authentication_config.set_property_value("SuppliedSecret", secret)

    device = instance.add_authenticated_device(device_info.connection_string, None, authentication_config)

    print("Authentication result:", "success" if device is not None else "failure")


if __name__ == "__main__":
    instance = build_instance()

    available_device_types = instance.available_device_types
    available_devices = instance.available_devices

    print('Discovered ' + str(len(available_devices)) + ' device(s):\n')
    for device_info in available_devices:
        device_type = resolve_device_type(available_device_types, device_info)
        authentication_config = create_authentication_config(instance, device_type)

        print(device_info.name + ':')
        daq_utils.print_indented('- Connection string: ' + device_info.connection_string, 1)
        daq_utils.print_indented('- Device type: ' + (device_type.id if device_type else '<unknown>'), 1)
        daq_utils.print_indented('- Remote: ' + str(bool(len(device_info.server_capabilities))), 1)
        print_authentication_support(authentication_config)

        if authentication_config is not None:
            process_authentication(instance, device_info, device_type, authentication_config)
        print()
