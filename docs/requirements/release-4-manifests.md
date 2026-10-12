# Release 4 — application manifest and package requirements

Release 4 uses one manually authored [JSON5 1.0.0](https://spec.json5.org/)
manifest per application package.
The schema below is a proposed AdaptivePi contract requiring review, not an
AUTOSAR schema or a decision already approved by the milestone issues.

## Requirement order

Review and complete MAN-001–009 first, then the supplemental launch specification
MAN-015, then MAN-010–014 parsing and validation. MAN-015 retains its stable ID
and appears at the end of this file, but belongs to the earlier specification
stage under #17. Every dependency is a lower-numbered entry in this file or an
explicitly earlier component stage in [the Release 4 overview](release-4-overview.md).
Each planned check verifies only its own observable contract and earlier prerequisites;
later integration evidence does not block completion of an earlier requirement.

## AP-R4-MAN-001 — Manifest syntax and version

- Status: Agreed
- Dependencies: Existing Release 3 core/log contracts only; no Release 4 prerequisite.
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: The manifest shall be a UTF-8 JSON5 1.0.0 document with an object root and a required `schema_version` whose value is a finite number with exact mathematical value `1`, regardless of its valid JSON5 numeric spelling and without string or boolean coercion; duplicate keys and unknown keys shall be invalid.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ManifestSpecification, §3.1.1.2, PDF pp. 14, RS_MANI_00006 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf); [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.2.7, PDF pp. 34, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: Documentation review and later `AP_R4_MAN_001_SyntaxAndVersion` cases for valid JSON5, including comments, trailing commas, single-quoted strings and unquoted member names; malformed JSON5; duplicates; unknown keys; missing or wrong-type versions; equivalent version spellings `1`, `1.0`, `1e0`, `+1` and `0x1`; rejected fractional values including `1.0000000000000001` without rounding to `1`; unsupported numeric versions; rejected strings, booleans, null, NaN and positive/negative infinity.
- Unit Tests:
- Deviation: JSON5 1.0.0, strict keys, and schema version 1 with value-based numeric validation are project choices. No ARXML parser or ARXML-to-JSON5 generator is provided.

## AP-R4-MAN-002 — Application package layout

- Status: Agreed
- Dependencies: `AP-R4-MAN-001`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: One self-contained application package shall contain `manifest.json5` at its root, its executable under `bin/`, a working directory under `work/`, and any packaged service unit under `systemd/`. Application-specific runtime libraries, configuration and resources required by the executable shall be bundled within the package root; operating-system runtime dependencies provided by the target image need not be bundled. Package loading shall take an explicit package-root path and shall not depend on a fixed installation prefix.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ManifestSpecification, §3.1.1.2, PDF p. 14, RS_MANI_00006 {DRAFT in AUTOSAR; application deployment context, not a directory-layout requirement}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf); [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.2.5, PDF p. 33, and §7.7.3.5, PDF p. 105, explanatory passages without requirement IDs: manifest/executable bundling and implementation-specific working folders](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf). No direct AUTOSAR requirement prescribes this self-contained directory layout or systemd integration; these are AdaptivePi design decisions.
- Verification: Documentation review of the package tree, application-specific runtime dependency inventory and external systemd unit registration; later `AP_R4_MAN_002_PackageLayout` checks for discovery, missing manifest and loading the same package layout from different package-root paths.
- Unit Tests:
- Deviation: Extension: the self-contained filesystem layout, bundled application-specific runtime assets and explicit package root are project-owned. Package installation and systemd unit registration are supported through the documented deployment workflow in EXEC-021; registered units may reside outside the package. Parsing/loading does not perform installation or process control. Full AUTOSAR Update and Configuration Management, update management and package signing remain excluded.

## AP-R4-MAN-003 — Required and optional manifest fields

- Status: In Review
- Dependencies: `AP-R4-MAN-001`; `AP-R4-MAN-002`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: The manifest shall require `application_id`, `executable`, `arguments`, `working_directory`, and `restart_policy`; it shall permit optional `environment` and `service_unit`. Identity, executable and working directory shall be strings, arguments an ordered array of strings, restart_policy an object, environment a string-to-string object defaulting to empty, and service_unit a string when present.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ManifestSpecification, §3.1.1.2, PDF pp. 14, RS_MANI_00006 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf); [AUTOSAR AP R23-11 TPS_ManifestSpecification, §8.3–8.3.1, PDF p. 389, TPS_MANI_01012, TPS_MANI_01013 {both DRAFT in AUTOSAR; contextual startup modelling, not the JSON5 manifest schema or C++ storage contract}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_TPS_ManifestSpecification.pdf)
- Verification: Documentation review and later `AP_R4_MAN_003_FieldShape` checks for each missing required field, wrong type, explicit null, empty arguments, and optional defaults.
- Unit Tests:
- Deviation: Only one startup configuration and one executable per package; no machine/function-group-dependent startup configurations.

## AP-R4-MAN-004 — Application identity

- Status: In Review
- Dependencies: `AP-R4-MAN-003`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: The application ID shall match `[A-Za-z][A-Za-z0-9_]*` and identify the single application handled by a manager instance.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ManifestSpecification, §3.1.1.2, PDF pp. 14, RS_MANI_00006 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf)
- Verification: Documentation review and later `AP_R4_MAN_004_Identity` cases for valid IDs, empty IDs, whitespace, separators, and punctuation.
- Unit Tests:
- Deviation: The ID syntax and one-application manager limit are project choices; no global registry or multi-instance naming model.

## AP-R4-MAN-005 — Package path rules

- Status: In Review
- Dependencies: `AP-R4-MAN-002`; `AP-R4-MAN-003`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: Executable and working-directory paths shall be relative to the package root, respectively under `bin/` and `work/`; absolute paths, empty paths, embedded NUL, and `.` or `..` components shall be invalid. Resolved paths, including symlink targets, shall remain inside the package root. The executable shall resolve to an executable regular file and the working directory to an existing directory.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ManifestSpecification, §3.1.1.2, PDF pp. 14, RS_MANI_00006 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf); [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.7.3.5, PDF pp. 105, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: Documentation review and later `AP_R4_MAN_005_Paths` cases for valid paths, traversal, missing/wrong-kind objects, permissions, and symlink escape.
- Unit Tests:
- Deviation: Filesystem containment and configured working directories are project decisions, not a claim of PSE51 filesystem semantics or protection against concurrent malicious filesystem mutation.

## AP-R4-MAN-006 — Argument and environment values

- Status: In Review
- Dependencies: `AP-R4-MAN-003`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: The manifest arguments array shall contain only configured arguments, excluding executable argument zero. Argument values shall preserve their order and bytes, including empty strings, spaces, quotes and shell metacharacters, and shall reject embedded NUL. Environment names shall match `[A-Za-z_][A-Za-z0-9_]*`; values may be empty strings but shall reject embedded NUL.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.4.3, PDF pp. 50, SWS_EM_01012, SWS_EM_01072, SWS_EM_01078](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.4.4, PDF pp. 50–51, SWS_EM_02246](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 TPS_ManifestSpecification, §8.3.1, PDF pp. 391, TPS_MANI_01209 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_TPS_ManifestSpecification.pdf)
- Verification: Documentation review and later `AP_R4_MAN_006_LaunchValues` cases for ordering, empty strings, special characters, invalid names and NUL.
- Unit Tests:
- Deviation: Only process-scoped overrides are supported; inherited systemd environment is not specified by this manifest. No Machine Manifest precedence model.

## AP-R4-MAN-007 — Bounded restart-policy configuration

- Status: In Review
- Dependencies: `AP-R4-MAN-003`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: The restart_policy object shall contain exactly `mode` and `max_attempts`: mode shall be `never` or `on_failure`; max_attempts shall be an integer in [0, 100], excluding JSON5 booleans. Mode never shall require max_attempts=0. The count denotes additional starts after unexpected termination within one explicit-start episode; the initial start is not counted.
- AUTOSAR source: [AUTOSAR AP R23-11 TPS_ManifestSpecification, §8.2, PDF pp. 385–386, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_TPS_ManifestSpecification.pdf); [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf)
- Verification: Documentation review and later `AP_R4_MAN_007_RestartPolicy` boundary cases for both modes, zero/100, negatives, fractions, over-limit values and inconsistent mode/count.
- Unit Tests:
- Deviation: The limit 100 and crash-restart episode are project choices. The source Process.numberOfRestartAttempts table concerns retries when startup fails; it is not an identical crash-recovery contract. No delay/backoff or automatic retry of an unsuccessful initial start.

## AP-R4-MAN-008 — Service-unit naming

- Status: In Review
- Dependencies: `AP-R4-MAN-003`; `AP-R4-MAN-004`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: A supplied service_unit shall match `[A-Za-z][A-Za-z0-9_-]*\.service`; omission shall select `adaptive-pi-<application_id>.service`. Names with path separators, whitespace, template markers or embedded NUL shall be invalid.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Documentation review and later `AP_R4_MAN_008_UnitName` cases for explicit/derived names and rejected forms.
- Unit Tests:
- Deviation: systemd unit naming is an AdaptivePi/Linux adapter decision, not an AUTOSAR interface.

## AP-R4-MAN-009 — Manifest example inventory

- Status: In Review
- Dependencies: `AP-R4-MAN-001`; `AP-R4-MAN-002`; `AP-R4-MAN-003`; `AP-R4-MAN-004`; `AP-R4-MAN-005`; `AP-R4-MAN-006`; `AP-R4-MAN-007`; `AP-R4-MAN-008`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: The manifest documentation shall include a complete valid JSON5 manifest and package example and individually labelled invalid examples covering each schema, field, path and policy rejection defined above.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Documentation review of examples against MAN-001–008; later parser tests shall reuse the same examples. Supplementary launch examples may illustrate the derived executable argument zero and the following manifest arguments, including an empty arguments array.
- Unit Tests:
- Deviation: Examples are manually authored JSON5 and package fixtures; no generated AUTOSAR artifacts.

## AP-R4-MAN-010 — Manifest error domain

- Status: In Review
- Dependencies: `AP-R4-MAN-001`; `AP-R4-MAN-002`; `AP-R4-MAN-003`; `AP-R4-MAN-004`; `AP-R4-MAN-005`; `AP-R4-MAN-006`; `AP-R4-MAN-007`; `AP-R4-MAN-008`
- Owning issue: [#18](https://github.com/Romanr92/adaptive-pi/issues/18)
- Requirement: Manifest operations shall provide a project-owned error domain and enum distinguishing unreadable manifest, invalid syntax, unsupported version, invalid field, invalid path and resource failure; domain identity and conversion shall follow the existing core domain contract in both exception configurations.
- AUTOSAR source: [AUTOSAR AP R23-11 EXP_InterfacesGuidelines, §3.1–3.1.2.2, PDF pp. 8–10, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf)
- Verification: `AP_R4_MAN_010_ErrorDomain` checks enum/value/domain conversion and configuration-specific exception conversion, using existing core infrastructure.
- Unit Tests:
- Deviation: Names and values are project-owned; no AUTOSAR-specific Manifest error domain or new core API is implied.

## AP-R4-MAN-011 — Owned configuration model

- Status: In Review
- Dependencies: `AP-R4-MAN-003`; `AP-R4-MAN-004`; `AP-R4-MAN-005`; `AP-R4-MAN-006`; `AP-R4-MAN-007`; `AP-R4-MAN-008`; `AP-R4-MAN-010`
- Owning issue: [#18](https://github.com/Romanr92/adaptive-pi/issues/18)
- Requirement: The manifest component shall provide an owned configuration model containing the application ID, package root, resolved executable and working directory, ordered arguments, environment overrides, service-unit name and restart policy, with no references into temporary parse buffers; this requirement defines storage and does not perform parsing or validation.
- AUTOSAR source: [AUTOSAR AP R23-11 TPS_ManifestSpecification, §8.3–8.3.1, PDF p. 389, TPS_MANI_01012, TPS_MANI_01013 {both DRAFT in AUTOSAR; contextual startup modelling, not the JSON5 manifest schema or C++ storage contract}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_TPS_ManifestSpecification.pdf); [AUTOSAR AP R23-11 EXP_InterfacesGuidelines, §3.1–3.1.2.2, PDF pp. 8–10, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf)
- Verification: `AP_R4_MAN_011_ConfigurationLifetime` checks contents after input buffers are destroyed and optional defaults.
- Unit Tests:
- Deviation: A manually authored C++ model replaces the AUTOSAR meta-model; ownership is a project contract.

## AP-R4-MAN-012 — Pure schema validation

- Status: In Review
- Dependencies: `AP-R4-MAN-001`; `AP-R4-MAN-003`; `AP-R4-MAN-004`; `AP-R4-MAN-006`; `AP-R4-MAN-007`; `AP-R4-MAN-008`; `AP-R4-MAN-010`; `AP-R4-MAN-011`
- Owning issue: [#18](https://github.com/Romanr92/adaptive-pi/issues/18)
- Requirement: Validation of parsed manifest values shall apply the syntax-independent schema and value rules above and return either complete candidate configuration data or a manifest ErrorCode, without filesystem access or process-control side effects.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_MAN_012_PureValidation` covers all value rules with in-memory inputs and asserts that no filesystem/controller dependency is needed.
- Unit Tests:
- Deviation: Separating pure validation supports host tests and bounded proofs; path existence and canonical resolution are the next independent stage.

## AP-R4-MAN-013 — Filesystem package validation

- Status: In Review
- Dependencies: `AP-R4-MAN-002`; `AP-R4-MAN-005`; `AP-R4-MAN-010`; `AP-R4-MAN-011`; `AP-R4-MAN-012`
- Owning issue: [#18](https://github.com/Romanr92/adaptive-pi/issues/18)
- Requirement: Package validation shall apply the path-resolution, containment, object-kind and executable-permission rules to a candidate configuration and return a complete validated configuration or a manifest ErrorCode.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_MAN_013_PackageValidation` uses temporary package trees for each path rule and resource/error branch; it shall not contact systemd.
- Unit Tests:
- Deviation: Host filesystem tests verify observed conditions; immutable deployment or stronger filesystem security is outside this release.

## AP-R4-MAN-014 — Manifest loading pipeline

- Status: In Review
- Dependencies: `AP-R4-MAN-002`; `AP-R4-MAN-009`; `AP-R4-MAN-010`; `AP-R4-MAN-011`; `AP-R4-MAN-012`; `AP-R4-MAN-013`
- Owning issue: [#18](https://github.com/Romanr92/adaptive-pi/issues/18)
- Requirement: Loading an explicit package root shall discover and read `manifest.json5`, parse its JSON5, invoke pure validation followed by package validation, and return `Result<validated configuration, ErrorCode>` without publishing partial configuration on failure.
- AUTOSAR source: [AUTOSAR AP R23-11 EXP_InterfacesGuidelines, §3.1–3.1.2.2, PDF pp. 8–10, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf); [AUTOSAR AP R23-11 RS_ManifestSpecification, §3.1.1.2, PDF pp. 14, RS_MANI_00006 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf)
- Verification: `AP_R4_MAN_014_LoadPackage` consumes the example inventory, unreadable files and malformed input; verifies Result error propagation and no process/controller calls in either exception mode.
- Unit Tests:
- Deviation: Parsing/discovery do not install a package, start a process, or use systemd. The parser library choice remains an implementation decision compatible with the default no-exceptions build.

## AP-R4-MAN-015 — Executable argument zero

- Status: In Review
- Dependencies: `AP-R4-MAN-005`; `AP-R4-MAN-006`
- Owning issue: [#17](https://github.com/Romanr92/adaptive-pi/issues/17)
- Requirement: The documented launch vector for a validated configuration shall place its resolved executable path at `argv[0]`, followed by every manifest argument unchanged and in array order beginning at `argv[1]`; for `n` manifest arguments, `argc` shall be `n+1`, including when the array is empty. Argument zero shall be derived from the validated executable and shall not require or permit another manifest field.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.4.3, PDF p. 50, SWS_EM_01072, SWS_EM_01078](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: Documentation review and `AP_R4_MAN_015_ArgumentVector` checks of documented configuration-value fixtures shall check the executable path, argument indexes, `argc`, empty arrays, empty argument strings and preserved special characters without implementing parsing, an owned C++ model, a controller or systemd. A later target argument-capture probe supplies supplementary integration evidence.
- Unit Tests:
- Deviation: None for argument indexing and ordering. AdaptivePi explicitly interprets the source's executable name as the validated resolved executable path; it does not introduce an application-selected argument-zero override. The launch-vector contract is documentation under #17; parser and backend implementation remain under #18/#19. The path interpretation is a project choice rather than an AUTOSAR-specified spelling.

This launch contract is a reusable foundation for Release 5 communication
applications ([#23](https://github.com/Romanr92/adaptive-pi/issues/23), with the
provider/client packages in [#28](https://github.com/Romanr92/adaptive-pi/issues/28))
and the Release 6 managed diagnostic-manager package
([#29](https://github.com/Romanr92/adaptive-pi/issues/29)). Their application APIs
and verification remain in their respective releases; they are not prerequisites
for this launch contract.

