# Credential Provider Framework — Usage Scenarios

A backlog of user stories and use-case scenarios for exercising `IDevice::createDefaultAuthenticationConfig`, `addAuthenticatedDevice`/`addStreaming`, and the credential provider framework end to end - meant to drive example/test implementations that verify the API is actually usable, not just that individual methods return the right type. Each scenario names the concrete API calls and expected outcome (return value, exception, or observable side effect) so it can be implemented directly, ideally against `credential_demo_module` the way `credential_providers.cpp` already does.

Persona used throughout: **an application developer** integrating openDAQ's credential framework into a client app or tool - deciding whether/how to offer authenticated connections to their users.

---

## 1. Discovery & feasibility

Before ever prompting a user or attempting a connection, an application needs to know what's even possible.

### 1.1 — Does this type support authentication at all?

As an application developer, I want to check whether a given device/streaming type supports authentication, so I can decide whether to offer an "authenticated connect" option in my UI at all.

- **Given** a type id whose module declares supported authentication descriptors and a default one for it (e.g. `"CredentialDemoDevice"`)
  **When** I call `instance.createDefaultAuthenticationConfig(typeId)`
  **Then** it succeeds and returns a config whose `"PayloadDescriptor"` selection has at least one candidate.
- **Given** a type id whose module declares no authentication support for it
  **When** I call `instance.createDefaultAuthenticationConfig(typeId)`
  **Then** it fails with `OPENDAQ_ERR_NOT_SUPPORTED` - not a crash, not an empty-but-successful config.
- **Given** a type id that names neither an available device type nor an available streaming type
  **When** I call `instance.createDefaultAuthenticationConfig(typeId)`
  **Then** it fails with `OPENDAQ_ERR_NOTFOUND`, distinguishable from the "not supported" case above (a developer needs to tell "unknown type" from "known type, no auth" apart - e.g. to detect a typo in `typeId` vs. correctly reporting "no auth available" to a user).
- **Given** the plain, anonymous path is what's wanted instead
  **When** I call `instance.addDevice(connectionString)` on a type that doesn't support authentication (or one that does, but I chose not to authenticate)
  **Then** it succeeds regardless - the two paths are fully independent (see the API reference, §3).

### 1.2 — Is authentication actually *usable* right now, given my registered providers?

As an application developer, I want to know not just whether a type *supports* authentication, but whether I can actually *complete* it with the providers currently registered on my instance - these are different questions, and the API doesn't collapse them into one boolean.

- **Given** a type supporting only a `String`-format method (e.g. `"Pin"`), and only a `FileCredentialProvider` (FilePath-only) registered
  **When** I call `instance.createDefaultAuthenticationConfig(typeId)` and check `hasProperty("CredentialProviderId")`
  **Then** its absence tells me, entirely client-side and without attempting a connection, that authentication would fail before ever calling `addAuthenticatedDevice` - `"CredentialProviderId"`'s live, format-filtered candidates (see the API reference, §1) mean this check no longer needs a separate hand-rolled cross-reference of `context.getCredentialProviders()` against the type's descriptors; the config itself already reflects the answer for whichever `"PayloadDescriptor"` is currently selected. For a *non-default* method, select it first (`SelectAuthenticationMethod`, §7) and re-check `hasProperty` - the live dependency between the two properties recomputes it for whatever is currently selected, not just the default.
- **Given** zero credential providers registered on the instance at all
  **When** I build the default config and call `addAuthenticatedDevice`
  **Then** it fails with `AuthenticationFailedException` ("no credential provider supporting a compatible payload format is registered") - a good negative-path example distinct from 1.1's "type doesn't support auth" case: here the *type* supports it, the *instance* just can't currently serve it.

### 1.3 — Feasibility for a device deeper in the tree, not just the root

As an application developer working with nested topologies, I want `createDefaultAuthenticationConfig` to work the same way when called on a sub-device, not only on the root instance.

- **Given** a device tree where an intermediate `IDevice` (not `Instance` itself) exposes `typeId` via its own `getAvailableDeviceTypes()`/`getAvailableStreamingTypes()`
  **When** I call `subDevice.createDefaultAuthenticationConfig(typeId)` directly on that sub-device rather than on `instance`
  **Then** it resolves and builds a config identically to calling it on the root - since `Instance` is a pure forwarder to `rootDevice`, and any `IDevice` implements this the same way via `GenericDevice`. Worth one explicit example beyond the always-root-instance calls in `credential_providers.cpp` today, to confirm the "parent device" framing (not "root device") actually holds.

---

## 2. Manual authenticated device — default config, unmodified

The user's original bullets, filled in.

### 2.1 — Default config + correct credentials, compatible provider registered

As an application developer, I want the simplest possible path to work: get the default config, hand it straight to `addAuthenticatedDevice` with no changes, and have it succeed when the user enters correct credentials.

- **Given** a `CmdLineCredentialProvider` (or equivalent, supporting the default method's format) registered on the instance
  **And** `authConfig = instance.createDefaultAuthenticationConfig(typeId)` used as-is (default `"PayloadDescriptor"` selection, default `"CredentialProviderId"` selection - see the gotcha in §4.1 below)
  **When** I call `instance.addAuthenticatedDevice(connectionString, nullptr, authConfig)` and supply correct credentials at the prompt
  **Then** the device is added, `device.getInfo()` is queryable, and `device.getAvailableDeviceTypes()`/other calls on it work normally.

### 2.2 — Default config + incorrect credentials

- **Given** the same setup as 2.1
  **When** incorrect credentials are supplied at the prompt
  **Then** `addAuthenticatedDevice` throws `AuthenticationFailedException`, the device is **not** added (verify `instance.getDevices()` doesn't contain it), and no partially-constructed device is left behind.

### 2.3 — Default config, no provider registered at all

- **Given** an instance built with no `addCredentialProvider` calls
  **When** I call `addAuthenticatedDevice` with a default config
  **Then** it throws `AuthenticationFailedException` before ever attempting to prompt for anything (no hang waiting on stdin) - the "no compatible provider" message, not a null-pointer/crash.

---

## 3. Manual authenticated device — supplied credential

### 3.1 — Correct supplied secret, provider registered and explicitly named

- **Given** `authConfig = instance.createDefaultAuthenticationConfig(typeId)` with the desired `"PayloadDescriptor"` selected (`SelectAuthenticationMethod`), `"CredentialProviderId"` explicitly set to `providerId`, and `"SuppliedSecret"` set to a correctly-shaped, correct `payload`
  **When** I call `addAuthenticatedDevice`
  **Then** authentication succeeds with **no interactive prompt at all** (verify via a non-interactive/scripted run - if the harness would hang on stdin, that's a bug or a wrong assumption about this path), and the named provider's `cacheCredentials(request, secret)` is invoked - confirmed indirectly by then making a *second*, separate connection attempt (same manufacturer/serial or canonical connection string) using only `requestCredentials` (no supplied secret) and observing it also completes without a prompt.

### 3.2 — Correct supplied secret, no provider named

- **Given** the same as 3.1 but `"CredentialProviderId"` never touched (left at its live default)
  **When** I call `addAuthenticatedDevice`
  **Then** authentication succeeds with no prompt (the supplied secret is used directly, no provider ever consulted per §8 step 3), **and** nothing is cached anywhere - confirmed by then authenticating a second, otherwise-identical connection *without* a supplied secret and observing that it **does** prompt (proving nothing leaked into a cache with no provider to own it).

### 3.3 — Incorrect supplied secret

- **Given** a supplied secret shaped correctly but with a wrong value (e.g. wrong password, wrong PIN)
  **When** I call `addAuthenticatedDevice`
  **Then** it throws `AuthenticationFailedException` - verifying that a caller can't bypass the module's own verification step just by constructing the payload shape correctly; supplying a secret skips the *provider*, not the *authenticator*.

### 3.4 — Malformed supplied secret (robustness)

As an application developer, I want a clear failure, not a confusing one, if I get the supplied-secret shape wrong.

- **Given** a supplied secret property object missing an expected property (e.g. a `KeyValuePairs` payload missing `"Password"`), or with an extra/misnamed one
  **When** I call `addAuthenticatedDevice`
  **Then** document (via the example) exactly what happens today - does it throw a clear, actionable error, or fail cryptically deep inside verification? This is worth an explicit example even if the answer turns out to be "not very friendly today," since it tells us whether `createDefaultPayload()` really needs to be used as the template (as documented) rather than treated as optional guidance.

---

## 4. Tuning the default config before use

Between "use the unmodified default" (§2) and "supply a secret directly" (§3), there's the actual expected common path: get the default, change one or two things, use it.

### 4.1 — The default config's provider selection is always format-compatible

As an application developer, I want the unmodified default config to just work: `"CredentialProviderId"`'s candidates are always filtered live to whichever format the currently-selected `"PayloadDescriptor"` needs, so the default (first compatible provider) can never be an incompatible pick the way an unfiltered "first registered" default could be.

- **Given** two providers registered, `FileCredentialProvider` (FilePath-only) registered before `CmdLineCredentialProvider` (all formats)
  **And** the type's default payload method is `KeyValuePairs` or `String` (not `FilePath`)
  **When** I use `instance.createDefaultAuthenticationConfig(typeId)` completely unmodified and call `addAuthenticatedDevice`
  **Then** it succeeds - `"CredentialProviderId"` defaulted to `CmdLineCredentialProvider` (the only one supporting the default method's format), not `FileCredentialProvider` just because it happened to register first. Worth keeping as a regression example: an earlier, unfiltered design genuinely broke this way (`demoUserNamePasswordAuthentication` failed with "the explicitly selected credential provider ... does not support the required payload format" before `"CredentialProviderId"` became live-filtered) - this scenario is what confirms it can't recur.

### 4.2 — Selecting a non-default payload method live-refilters the provider candidates

- **Given** the default config lists more than one `"PayloadDescriptor"` candidate, each needing a different format, with providers registered that don't all support every format
  **When** I switch the selection to a non-default one (matching by candidate `Id`, the pattern already in `credential_providers.cpp`'s `SelectAuthenticationMethod`) and then inspect `"CredentialProviderId"`'s current candidates (`getProperty("CredentialProviderId").getSelectionValues()`)
  **Then** the candidate list reflects the *new* method's format, not the old one - confirming the dependency between the two properties is genuinely live (re-queried from `Context`), not just correct at construction. Calling `addAuthenticatedDevice` afterward requests/verifies the *new* method's credentials, not the default's.
  **And**, if no registered provider supports the newly-selected format, `hasProperty("CredentialProviderId")` becomes `false` - the property is removed, not left present with a stale or empty candidate list.

### 4.3 — Selecting a non-default credential provider explicitly

- **Given** two format-compatible providers registered
  **When** I set `"CredentialProviderId"` (via `setPropertySelectionValue`, since it's a Selection when the config was built with a `Context`) to the *second* one before calling `addAuthenticatedDevice`
  **Then** the second provider - not the auto/default-selected first one - is the one whose `requestCredentials`/`cacheCredentials` gets called; verify by checking which provider's cache holds the entry afterward (§3.1's technique), or by giving each provider observably different prompts/behavior.
  **And** this choice is "sticky" across a later `"PayloadDescriptor"` change: switching to a different method that the second provider *also* supports keeps it selected (not silently reverting to whichever provider is first in the new candidate list) - confirming the config remembers the caller's last explicit provider choice, not just the current one.

### 4.4 — Explicit provider id that's format-incompatible or unregistered is now unreachable through the public API

`"CredentialProviderId"`'s candidates are always pre-filtered live to providers compatible with the currently-selected `"PayloadDescriptor"` (see §4.1/§4.2) - there is no longer any config shape through which a caller can select a format-incompatible or nonexistent provider id at all, so `FindMatchingCredentialProvider`'s corresponding "explicit id names no compatible/no registered provider" failure paths are module-internal defensive code only, not something an application-level scenario can still exercise.

- **Given** the removal of `IAuthenticationConfigBuilder` (which previously exposed a plain-string `"CredentialProviderId"` with no such filtering)
  **Then** these two failure paths have no remaining application-reachable trigger - worth a code-level note (not a runnable example) so a future change to this filtering doesn't silently reopen this gap without matching test coverage.

### 4.6 — An incompatible `"SuppliedSecret"` write is rejected

- **Given** a config with `"PayloadDescriptor"` selected to a `KeyValuePairs`-format method (e.g. `UserNamePassword`)
  **When** I call `setPropertyValue("SuppliedSecret", ...)` with an object shaped for a *different* format (e.g. a single `"Secret"` property, matching `String`/`FilePath` instead)
  **Then** the write throws and `"SuppliedSecret"` is not set - confirming validation happens against the object's actual property names/count, not just "any object goes."

### 4.7 — Changing the selected authentication method silently clears an incompatible `"SuppliedSecret"`

- **Given** a config with a valid `"SuppliedSecret"` set for the currently-selected `"PayloadDescriptor"`
  **When** I switch `"PayloadDescriptor"` to a different method whose `createDefaultPayload()` shape doesn't match the existing secret
  **Then** the selection change succeeds (no exception) and `hasProperty("SuppliedSecret")` becomes `false` afterward - the stale secret is silently cleared, not left in place mismatched with the new selection, and not blocking the method switch. Calling `addAuthenticatedDevice` afterward with no secret re-supplied falls through to the normal provider-based path (§2), prompting or using the (now re-filtered) `"CredentialProviderId"` selection.

---

## 5. Credential provider caching

### 5.1 — Same context, same provider, same format → cache hit

- **Given** a device authenticated via a caching-capable provider and format (e.g. `CmdLineCredentialProvider` + `FilePath`)
  **When** a *second* connection sharing the same manufacturer/serial (or, absent those, the same canonical connection string - e.g. a streaming attach to the same device) is authenticated via the same provider and format
  **Then** no prompt occurs the second time - this is the scenario `demoCachedFilePathCredentialAcrossDeviceAndStreaming` already demonstrates; worth keeping as a named, minimal example independent of the bigger demo.

### 5.2 — Different context → cache miss (isolation)

The inverse of 5.1, and just as important to verify explicitly.

- **Given** the same setup as 5.1
  **When** a connection with a *different* manufacturer/serial pair (or a different canonical connection string) is authenticated via the same provider and format
  **Then** it prompts independently - confirming the cache key genuinely discriminates by connection identity and doesn't leak credentials across unrelated devices.

### 5.3 — Non-caching provider or format

- **Given** `FileCredentialProvider` (documented as never caching) or a `KeyValuePairs`/`String` request against `CmdLineCredentialProvider` (only `FilePath` is cached)
  **When** the same context is authenticated twice
  **Then** it prompts both times - confirming caching is opt-in per provider/format, not a blanket assumption an application can rely on for every combination.

---

## 6. Streaming authentication

### 6.1 — Streaming authenticated independently of its device

- **Given** a device connected via one method (or not authenticated at all)
  **When** a streaming connection to it is added via `device.addStreaming(connectionString, nullptr, streamingAuthConfig)` using a *different* method/provider
  **Then** both succeed independently - the streaming leg's authentication outcome (success or failure) is unaffected by how the device itself was connected, and vice versa.

### 6.2 — Auto-attached streaming is always unauthenticated

As an application developer relying on `PrioritizedStreamingProtocols`/`AutomaticallyConnectStreaming`, I want to know this path never authenticates on my behalf, so I don't assume protection I don't have.

- **Given** a device added with auto-attach streaming config enabled, to a streaming type that *does* support authentication
  **When** the device connects
  **Then** the auto-attached streaming source is present and working, but was never asked for credentials of any kind - verified by registering *no* compatible provider at all and confirming auto-attach still succeeds (where a manual `addStreaming` with an auth config would instead fail per §2.3).

---

## 7. Save & load

The user's last bullet, filled in with the fully custom persistence model: `AuthenticationConfigImpl`'s own `serialize()` writes only the component type id and the selected payload method id - never `"CredentialProviderId"`, never `"SuppliedSecret"`, regardless of whether they're currently present. Reload re-resolves the type against the *new* instance's `Context` and rebuilds `"CredentialProviderId"` exactly as a brand-new `createDefaultAuthenticationConfig` call would - live-filtered by the saved method's format, defaulting to the first compatible provider. There is no "matching by saved provider id" at all.

### 7.1 — Reload with any compatible provider registered (not necessarily the same one)

- **Given** an authenticated device saved via `instance.saveConfiguration()`, originally authenticated through provider A
  **When** a new instance registers a *different* provider B - same format support, different id/class - then calls `loadConfiguration(savedConfiguration)`
  **Then** the device reconnects through provider B without issue - confirming reload re-resolves `"CredentialProviderId"` fresh from the new instance's `Context` rather than requiring a provider matching any saved id (there is none saved to match). Re-authenticates from scratch through the normal `requestCredentials` path - prompting again unless B happens to have something cached for this context (§5.1).

### 7.2 — Reload with no compatible provider registered

- **Given** the same saved configuration
  **When** the new instance registers no provider at all, or one that doesn't support the persisted method's format
  **Then** `loadConfiguration` fails to reconnect the device (confirm via `reloadedInstance.getDevices()` being empty, matching `demoPinAuthenticationAndReload`'s own check) - not a silent, disconnected-but-present device. (This is also what `"CredentialProviderId"` being entirely absent on the rebuilt config would look like internally - no compatible provider to select.)

### 7.3 — Reload with a stale saved method id

- **Given** a saved configuration whose method id is no longer among the type's *currently* supported descriptors (e.g. the module was updated and dropped or renamed that method), or whose saved type id no longer resolves to any available device/streaming type at all
  **When** the new instance calls `loadConfiguration(savedConfiguration)`
  **Then** the reload fails hard (custom `Deserialize` throws) rather than silently substituting the type's current default method - confirming a reload never silently authenticates via a method the user didn't actually choose.

### 7.4 — A supplied-secret device does not skip the prompt on reload

- **Given** a device originally authenticated via `setSuppliedSecret` (§3.1/3.2)
  **When** it's saved and reloaded into a new instance with a compatible provider registered
  **Then** the reload prompts (or fails, if no provider is registered) exactly as a from-scratch connection would - confirming the supplied secret never survives serialization at all, only the type id and payload method id do.

---

## 8. Boundary & negative checks worth having somewhere, even briefly

- Calling `createDefaultAuthenticationConfig` with a `typeId` that's valid but for a component sort with no notion of authentication at all (e.g. a function block type, if one is ever passed) - confirm it's treated the same as "unsupported" (`OPENDAQ_ERR_NOT_SUPPORTED`) rather than something type-confused.
- Calling `addAuthenticatedDevice` with `authenticationConfig = nullptr` against a type that supports authentication - confirm this is rejected clearly (`AuthenticationFailedException`, from `Module::requestCredentials`'s own check, before a module's `onCreateAuthenticatedDevice` is ever invoked), distinct from silently falling back to the anonymous path.
- Two authentication attempts to the *same* connection string in quick succession without an intervening `removeDevice` (e.g. accidental double-click in a UI) - confirm the second attempt's failure/success mode is well-defined rather than racing the first.
