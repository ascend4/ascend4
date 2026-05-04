# ASCEND Security Notes

ASCEND models and extension mechanisms are intended for trusted scientific and
engineering workflows. Loading an ASCEND model can cause user-selected ASCEND
libraries, solver plugins, external methods, Python extension methods, and other
native components to be loaded into the ASCEND process.

## Privileged Execution

ASCEND is not intended to run as `root`, setuid/setgid, or with mismatched real
and effective user or group IDs on Unix-like systems. In those cases ASCEND
refuses operations that load or execute model-controlled native code, including:

* dynamic library loading through `Asc_DynamicLoad`;
* Python external methods through `extpy`;
* binary-token code generation and loading.

The launcher scripts also refuse privileged execution early where platform
support is available.

## User Imports and Extension Code

Importing user models and user-built extension libraries is an intentional
ASCEND feature. ASCEND does not generally restrict users to a fixed installation
tree, because normal workflows include local model libraries and locally built
extensions.

Do not run untrusted ASCEND models or extension libraries in a privileged or
high-trust process. Use normal OS isolation, separate user accounts, containers,
or virtual machines when evaluating untrusted input.

## Binary Tokens

Binary tokens are an optional/developer feature that compiles generated C code
and loads the resulting native library into the ASCEND process. The feature is
off unless enabled by host/frontend code through the binary-token API.

When binary tokens are enabled:

* temporary source and library files are created in a private temporary
  directory;
* generated files are cleaned up when housekeeping is enabled;
* privileged Unix execution is refused;
* custom binary-token build commands are considered unsafe/developer-only,
  because legacy custom commands are executed through the shell.

Environment variables such as `ASCENDBTINC` and `ASCENDBTLIB` configure
binary-token include and library paths after the feature is enabled. They should
not be treated as an authorization mechanism.

## Reporting Security Issues

If you find a vulnerability, report it privately to the ASCEND maintainers
before public disclosure. Include the platform, build configuration, ASCEND
version or commit, and a minimal reproducer where possible.
